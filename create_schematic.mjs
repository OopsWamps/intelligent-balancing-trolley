// 平衡车底板原理图自动创建脚本
// 通过 EasyEDA API 在当前原理图页面上创建完整原理图

const LIB = "0819f05c4eef4c71ace90d822a990e87";

// 辅助函数：搜索元件并返回第一个结果
async function findDevice(keyword) {
  const results = await eda.lib_Device.search(keyword, undefined, undefined, undefined, 5, 1);
  return results.length > 0 ? results[0] : null;
}

// 辅助函数：放置元件并修改位号
async function placeComponent(deviceUuid, x, y, designator, name, rotation) {
  try {
    const comp = await eda.sch_PrimitiveComponent.create(
      { uuid: deviceUuid, libraryUuid: LIB },
      x, y, undefined, rotation || 0
    );
    if (comp) {
      const modProps = { designator: designator };
      if (name) modProps.name = name;
      await eda.sch_PrimitiveComponent.modify(comp.primitiveId, modProps);
      return comp;
    }
  } catch (e) {
    console.log(`Error placing ${designator}: ${e.message}`);
  }
  return null;
}

// 辅助函数：放置电源标志
async function placePowerFlag(type, net, x, y, rotation) {
  try {
    return await eda.sch_PrimitiveComponent.createNetFlag(type, net, x, y, rotation || 0);
  } catch (e) {
    console.log(`Error placing power flag ${net}: ${e.message}`);
  }
  return null;
}

// 辅助函数：放置网络端口
async function placeNetPort(direction, net, x, y, rotation) {
  try {
    return await eda.sch_PrimitiveComponent.createNetPort(direction, net, x, y, rotation || 0);
  } catch (e) {
    console.log(`Error placing net port ${net}: ${e.message}`);
  }
  return null;
}

// 辅助函数：绘制导线
async function drawWire(points, net) {
  try {
    return await eda.sch_PrimitiveWire.create(points, net);
  } catch (e) {
    console.log(`Error drawing wire: ${e.message}`);
  }
  return null;
}

// 辅助函数：添加文本
async function addText(x, y, content) {
  try {
    return await eda.sch_PrimitiveText.create(x, y, content);
  } catch (e) {
    console.log(`Error adding text: ${e.message}`);
  }
  return null;
}

// ==================== 主流程 ====================

const results = { placed: [], errors: [] };

// Phase 1: 搜索元件
console.log("Phase 1: Searching components...");

// 已知可用的排针 UUID（通过 "Pin Header 1xN" 搜索验证）
const HEADER_UUIDS = {
  "1x2": "393a2bfc482644e2956f645e082fd8cd",
  "1x4": "d602b633547c4368afabbe4d4ba5d108",
  "1x6": "3d79b1575e1f4ba082581a8f1fa2e9da",
  "1x8": "4fc91ab1ca30459392244a64bd3ecd1b",
  "1x10": "fd511fb0ae8b4a0d90aace538fcaa7bb",
  "1x12": null,  // 需要搜索
  "1x15": null,  // 需要搜索
  "2x15": "82565ac4f07f49be834154e513190b29",
};

// 搜索缺失的排针规格
for (const size of ["1x12", "1x15"]) {
  const dev = await findDevice(`Pin Header ${size}`);
  if (dev) {
    HEADER_UUIDS[size] = dev.uuid;
  }
}

// 搜索其他元件
let TB6612_UUID = "0e6600f45b16489e9666016017267cfb";
let MPU6050_UUID = "eefecb57ac144fb4b62f643e140a012a";
let CAP_100NF_UUID = "f4031ec0d8584b6b8c275c860e9e1d81";
let CAP_220UF_UUID = "8d434490c3f84b97ad46fb3d8956ae05";
let SW_UUID = "cff07d923b95482c818b184e98b80db7";
let DC005_UUID = "aa38039ffb7144df89b006eb6329c5ad";

// Phase 2: 放置元件
console.log("Phase 2: Placing components...");

// 元件放置列表：[设计号, UUID, x, y, 名称, 旋转角度]
const components = [
  // 电源部分 (顶部)
  ["J9",  DC005_UUID,     80,  100, "DC-005",   0],
  ["SW1", SW_UUID,        220, 100, "SW-SPDT",  0],
  ["C2",  CAP_220UF_UUID, 350, 100, "220uF",    0],
  ["J6",  HEADER_UUIDS["1x4"],  500, 100, "DF",    0],
  ["C3",  CAP_100NF_UUID, 620, 80,  "104",      0],
  ["C4",  CAP_100NF_UUID, 700, 80,  "104",      0],
  ["J5",  HEADER_UUIDS["1x10"], 820, 100, "12TO5", 0],

  // 主控部分 (中部) - Forest S1 STM32 用2个1x15排针表示
  ["J1",  HEADER_UUIDS["1x15"] || HEADER_UUIDS["1x12"], 300, 300, "Forest_S1_L", 0],
  ["J1B", HEADER_UUIDS["1x15"] || HEADER_UUIDS["1x12"], 500, 300, "Forest_S1_R", 0],

  // TB6612电机驱动
  ["tb1", TB6612_UUID,    780, 300, "TB6612",   0],
  ["C1",  CAP_100NF_UUID, 780, 200, "104",      0],

  // MPU6050
  ["MPU1", HEADER_UUIDS["1x8"], 400, 620, "MPU6050", 0],

  // 左侧连接器
  ["J8",  HEADER_UUIDS["1x6"], 80, 300, "WIFI",    0],
  ["J7",  HEADER_UUIDS["1x6"], 80, 420, "BT",      0],
  ["J3",  HEADER_UUIDS["1x4"], 80, 540, "4G",      0],
  ["P1",  HEADER_UUIDS["1x6"], 80, 640, "Header6", 0],

  // 右侧/底部连接器
  ["J10", HEADER_UUIDS["1x6"], 950, 200, "Header6", 0],
  ["P2",  HEADER_UUIDS["1x7"] || HEADER_UUIDS["1x6"], 950, 340, "Header7", 0],
  ["J4",  HEADER_UUIDS["1x4"], 950, 460, "Header4", 0],
  ["P3",  HEADER_UUIDS["1x12"], 950, 560, "Header12", 0],
  ["P4",  HEADER_UUIDS["1x6"], 700, 620, "Header6", 0],
  ["J2",  HEADER_UUIDS["1x2"], 580, 620, "Header2", 0],
];

const placedComponents = {};

for (const [des, uuid, x, y, name, rot] of components) {
  if (!uuid) {
    results.errors.push(`No UUID for ${des}`);
    continue;
  }
  const comp = await placeComponent(uuid, x, y, des, name, rot);
  if (comp) {
    placedComponents[des] = comp.primitiveId;
    results.placed.push(des);
  } else {
    results.errors.push(`Failed to place ${des}`);
  }
}

// Phase 3: 添加电源标志
console.log("Phase 3: Adding power flags...");

// 12V 电源
await placePowerFlag("Power", "12V", 100, 70, 0);
await placePowerFlag("Power", "12V", 820, 70, 0);

// 5V 电源
await placePowerFlag("Power", "5V", 650, 70, 0);
await placePowerFlag("Power", "5V", 300, 270, 0);

// 3V3 电源
await placePowerFlag("Power", "3V3", 500, 270, 0);

// GND
await placePowerFlag("Ground", "GND", 100, 160, 0);
await placePowerFlag("Ground", "GND", 650, 160, 0);
await placePowerFlag("Ground", "GND", 400, 700, 0);

// Phase 4: 添加文本标注
console.log("Phase 4: Adding text annotations...");

await addText(80, 280, "WIFI模块");
await addText(80, 400, "蓝牙模块");
await addText(80, 520, "4G模块");
await addText(300, 280, "Forest S1 STM32最小系统");
await addText(780, 280, "TB6612电机驱动");
await addText(400, 600, "MPU6050");

results.totalPlaced = results.placed.length;
results.totalErrors = results.errors.length;

return results;
