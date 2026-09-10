"use strict";

const VID = 0xcafe;
const PID = 0x4113;
const INTERFACE = 1;
const ENDPOINT = 2;
const HEADER_SIZE = 36;
const CHUNK_SIZE = 512;
const commands = { begin: 1, data: 2, end: 3, abort: 4, status: 5 };
const statusText = [
  "成功", "未知命令", "设备状态错误", "数据长度错误",
  "数据偏移错误", "SPI Flash 操作失败", "固件校验失败",
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
};

let device = null;
let busy = false;

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
  elements.connect.textContent = connected ? "断开设备" : "连接设备";
  elements.connect.disabled = busy;
  elements.firmware.disabled = busy;
  elements.upgrade.disabled = busy || !connected || !elements.firmware.files[0];
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
    filters: [{ vendorId: VID, productId: PID }],
  });
  await device.open();
  if (!device.configuration) await device.selectConfiguration(1);
  await device.claimInterface(INTERFACE);
  elements.deviceName.textContent = device.productName || "CH585 OTA Device";
  setState("已连接", "ready");
  log(`已连接 ${elements.deviceName.textContent}`);
}

async function disconnect() {
  if (device?.opened) {
    try { await device.releaseInterface(INTERFACE); } catch (_) {}
    await device.close();
  }
  device = null;
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
  const received = await device.transferIn(ENDPOINT, 16);
  if (received.status !== "ok" || received.data.byteLength !== 16) {
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
  };
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
    setState("连接失败", "error");
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
    setState("已断开");
    updateControls();
  }
});

updateControls();
