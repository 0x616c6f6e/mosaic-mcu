"use strict";

const VID = 0xcafe;
const PIDS = [0x4113, 0x4114];
const ENDPOINT = 2;
const HEADER_SIZE = 36;
const CHUNK_SIZE = 512;
const CONFIG_DATA_SIZE = 44;
const logLevels = ["debug", "info", "warn", "error", "none"];
const commands = {
  begin: 1,
  data: 2,
  end: 3,
  abort: 4,
  status: 5,
  getConfig: 6,
  setConfig: 7,
};
const statusText = [
  "成功", "未知命令", "设备状态错误", "数据长度错误",
  "数据偏移错误", "SPI Flash 操作失败", "固件校验失败", "配置无效",
];

const elements = {
  connect: document.querySelector("#connect"),
  upgrade: document.querySelector("#upgrade"),
  firmware: document.querySelector("#firmware"),
  state: document.querySelector("#device-state"),
  deviceName: document.querySelector("#device-name"),
  fileName: document.querySelector("#file-name"),
  fileSize: document.querySelector("#file-size"),
  progress: document.querySelector(".progress"),
  progressBar: document.querySelector("#progress-bar"),
  progressText: document.querySelector("#progress-text"),
  log: document.querySelector("#log"),
  clearLog: document.querySelector("#clear-log"),
  firmwareTab: document.querySelector("#firmware-tab"),
  configTab: document.querySelector("#config-tab"),
  firmwarePanel: document.querySelector("#firmware-panel"),
  configPanel: document.querySelector("#config-panel"),
  configName: document.querySelector("#config-name"),
  configLogLevel: document.querySelector("#config-log-level"),
  configIdleTimeout: document.querySelector("#config-idle-timeout"),
  configRebootDelay: document.querySelector("#config-reboot-delay"),
  configDiskVisible: document.querySelector("#config-disk-visible"),
  configStatus: document.querySelector("#config-status"),
  saveConfig: document.querySelector("#save-config"),
};

let device = null;
let claimedInterface = null;
let busy = false;
let configLoaded = false;

function log(message) {
  const time = new Date().toLocaleTimeString("zh-CN", { hour12: false });
  elements.log.textContent += `[${time}] ${message}\n`;
  elements.log.scrollTop = elements.log.scrollHeight;
}

function setState(text, kind = "idle") {
  elements.state.textContent = text;
  elements.state.dataset.kind = kind;
}

function updateControls() {
  const connected = Boolean(device?.opened);
  const configReady = connected && configLoaded && !busy;
  elements.connect.textContent = connected ? "断开设备" : "连接设备";
  elements.connect.disabled = busy;
  elements.firmware.disabled = busy;
  elements.upgrade.disabled = busy || !connected || !elements.firmware.files[0];
  for (const field of [elements.configName, elements.configLogLevel,
    elements.configIdleTimeout, elements.configRebootDelay,
    elements.configDiskVisible]) {
    field.disabled = !configReady;
  }
  elements.saveConfig.disabled = !configReady;
}

function showTab(name) {
  const showFirmware = name === "firmware";
  elements.firmwareTab.classList.toggle("active", showFirmware);
  elements.firmwareTab.setAttribute("aria-selected", String(showFirmware));
  elements.configTab.classList.toggle("active", !showFirmware);
  elements.configTab.setAttribute("aria-selected", String(!showFirmware));
  elements.firmwarePanel.hidden = !showFirmware;
  elements.configPanel.hidden = showFirmware;
}

function setProgress(value, text) {
  const percent = Math.max(0, Math.min(100, value));
  elements.progressBar.style.width = `${percent}%`;
  elements.progress.setAttribute("aria-valuenow", String(Math.round(percent)));
  elements.progressText.textContent = text;
}

function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KiB`;
  return `${(bytes / 1024 / 1024).toFixed(2)} MiB`;
}

async function connect() {
  if (!("usb" in navigator)) throw new Error("当前浏览器不支持 WebUSB");
  device = await navigator.usb.requestDevice({
    filters: PIDS.map((productId) => ({ vendorId: VID, productId })),
  });
  await device.open();
  if (!device.configuration) await device.selectConfiguration(1);
  const vendorInterface = device.configuration.interfaces.find((item) =>
    item.alternates.some((alternate) => alternate.interfaceClass === 0xff));
  if (!vendorInterface) throw new Error("设备缺少 WebUSB 接口");
  claimedInterface = vendorInterface.interfaceNumber;
  await device.claimInterface(claimedInterface);
  elements.deviceName.textContent = device.productName || "Mosaic Keyboard";
  setState("已连接", "ready");
  log(`已连接 ${elements.deviceName.textContent}`);
  elements.configStatus.textContent = "正在读取配置";
  try {
    await loadConfiguration();
  } catch (error) {
    configLoaded = false;
    elements.configStatus.textContent = "配置读取失败";
    log(`配置读取失败：${error.message}`);
  }
}

async function disconnect() {
  if (device?.opened) {
    try { await device.releaseInterface(claimedInterface); } catch (_) {}
    await device.close();
  }
  device = null;
  claimedInterface = null;
  configLoaded = false;
  elements.configStatus.textContent = "连接设备后读取配置";
  setState("未连接");
  log("设备已断开");
}

function validatePackage(bytes) {
  if (bytes.byteLength < HEADER_SIZE) throw new Error("OTA 文件过小");
  const view = new DataView(bytes);
  if (view.getUint32(0, true) !== 0x41544f4d) throw new Error("OTA 文件标识无效");
  if (view.getUint32(8, true) !== HEADER_SIZE) throw new Error("OTA 包头长度无效");
  if (view.getUint32(12, true) !== 0x585) throw new Error("固件目标不是 CH585");
  if (view.getUint32(16, true) !== 0x10000) throw new Error("固件加载地址无效");
  const imageSize = view.getUint32(20, true);
  if (HEADER_SIZE + imageSize !== bytes.byteLength) throw new Error("OTA 文件长度无效");
  return {
    version: view.getUint32(28, true),
    imageSize,
  };
}

async function exchange(command, argument = 0, payload = new Uint8Array()) {
  const frame = new Uint8Array(12 + payload.byteLength);
  const view = new DataView(frame.buffer);
  frame.set([0x57, 0x4f, 0x54, 0x41], 0);
  frame[4] = command;
  view.setUint16(6, payload.byteLength, true);
  view.setUint32(8, argument, true);
  frame.set(payload, 12);

  const sent = await device.transferOut(ENDPOINT, frame);
  if (sent.status !== "ok" || sent.bytesWritten !== frame.byteLength) {
    throw new Error("USB 数据发送失败");
  }
  const received = await device.transferIn(ENDPOINT, 64);
  if (received.status !== "ok" || received.data.byteLength < 16) {
    throw new Error("USB 应答接收失败");
  }
  const response = received.data;
  if (response.getUint32(0, true) !== 0x41544f52 || response.getUint8(4) !== command) {
    throw new Error("设备返回了无效应答");
  }
  const status = response.getUint8(5);
  if (status !== 0) throw new Error(statusText[status] || `设备错误 ${status}`);
  return {
    received: response.getUint32(8, true),
    total: response.getUint32(12, true),
    payload: new Uint8Array(
      response.buffer,
      response.byteOffset + 16,
      response.byteLength - 16,
    ).slice(),
  };
}

function decodeConfiguration(payload) {
  if (payload.byteLength !== CONFIG_DATA_SIZE) {
    throw new Error("设备返回的配置长度无效");
  }
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const version = view.getUint8(0);
  const diskVisible = view.getUint8(1);
  const logLevel = view.getUint8(2);
  const nameLength = view.getUint8(3);
  if (version !== 1 || diskVisible > 1 || logLevel >= logLevels.length ||
      nameLength === 0 || nameLength > 31) {
    throw new Error("设备返回的配置内容无效");
  }
  return {
    deviceName: new TextDecoder().decode(payload.slice(12, 12 + nameLength)),
    diskVisible: diskVisible === 1,
    logLevel: logLevels[logLevel],
    idleTimeout: view.getUint32(4, true),
    rebootDelay: view.getUint32(8, true),
  };
}

async function loadConfiguration() {
  const response = await exchange(commands.getConfig);
  const config = decodeConfiguration(response.payload);
  elements.configName.value = config.deviceName;
  elements.configDiskVisible.checked = config.diskVisible;
  elements.configLogLevel.value = config.logLevel;
  elements.configIdleTimeout.value = String(config.idleTimeout);
  elements.configRebootDelay.value = String(config.rebootDelay);
  elements.configStatus.textContent = "配置已同步";
  configLoaded = true;
  updateControls();
  log("设备配置读取完成");
}

async function saveConfiguration() {
  if (!elements.configPanel.reportValidity()) return;
  const config = {
    schema_version: 2,
    device_name: elements.configName.value,
    usb_disk_visible: elements.configDiskVisible.checked,
    log_level: elements.configLogLevel.value,
    usb_idle_timeout_ms: Number(elements.configIdleTimeout.value),
    webusb_reboot_delay_ms: Number(elements.configRebootDelay.value),
  };
  const payload = new TextEncoder().encode(`${JSON.stringify(config, null, 2)}\r\n`);
  if (payload.byteLength > CHUNK_SIZE) throw new Error("配置数据过长");
  await exchange(commands.setConfig, 0, payload);
  elements.configStatus.textContent = "配置已提交，设备正在重新连接";
  log(`配置已提交，USB 存储盘 ${config.usb_disk_visible ? "显示" : "隐藏"}`);
}

async function upgrade() {
  const file = elements.firmware.files[0];
  const buffer = await file.arrayBuffer();
  const metadata = validatePackage(buffer);
  const bytes = new Uint8Array(buffer);

  log(`开始升级，版本 ${metadata.version}，镜像 ${formatBytes(metadata.imageSize)}`);
  setProgress(0, "正在初始化暂存区");
  await exchange(commands.begin, bytes.byteLength);

  for (let offset = 0; offset < bytes.byteLength; offset += CHUNK_SIZE) {
    const chunk = bytes.subarray(offset, Math.min(offset + CHUNK_SIZE, bytes.byteLength));
    const response = await exchange(commands.data, offset, chunk);
    const percent = response.received * 100 / response.total;
    setProgress(percent, `正在传输 ${Math.round(percent)}%`);
  }

  setProgress(100, "正在校验固件");
  await exchange(commands.end);
  setProgress(100, "固件已提交，设备正在重启安装");
  setState("正在重启", "ready");
  log("固件校验通过，设备将进入 bootloader 安装");
}

elements.connect.addEventListener("click", async () => {
  try {
    if (device?.opened) await disconnect();
    else await connect();
  } catch (error) {
    const failedDevice = device;

    device = null;
    claimedInterface = null;
    configLoaded = false;
    if (failedDevice?.opened) {
      try { await failedDevice.close(); } catch (_) {}
    }
    setState("连接失败", "error");
    elements.configStatus.textContent = "配置不可用";
    log(`连接失败：${error.message}`);
  } finally {
    updateControls();
  }
});

elements.firmware.addEventListener("change", () => {
  const file = elements.firmware.files[0];
  elements.fileName.textContent = file?.name || "尚未选择文件";
  elements.fileSize.textContent = file ? formatBytes(file.size) : "0 B";
  setProgress(0, file ? "可以开始升级" : "等待连接设备和选择固件");
  updateControls();
});

elements.firmwareTab.addEventListener("click", () => showTab("firmware"));
elements.configTab.addEventListener("click", () => showTab("config"));

elements.configPanel.addEventListener("submit", async (event) => {
  event.preventDefault();
  busy = true;
  updateControls();
  elements.configStatus.textContent = "正在保存配置";
  try {
    await saveConfiguration();
  } catch (error) {
    elements.configStatus.textContent = "配置保存失败";
    log(`配置保存失败：${error.message}`);
  } finally {
    busy = false;
    updateControls();
  }
});

elements.upgrade.addEventListener("click", async () => {
  busy = true;
  updateControls();
  try {
    await upgrade();
  } catch (error) {
    setState("升级失败", "error");
    setProgress(0, error.message);
    log(`升级失败：${error.message}`);
    try { await exchange(commands.abort); } catch (_) {}
  } finally {
    busy = false;
    updateControls();
  }
});

elements.clearLog.addEventListener("click", () => { elements.log.textContent = ""; });

navigator.usb?.addEventListener("disconnect", (event) => {
  if (event.device === device) {
    device = null;
    claimedInterface = null;
    configLoaded = false;
    setState("已断开");
    updateControls();
  }
});

updateControls();
