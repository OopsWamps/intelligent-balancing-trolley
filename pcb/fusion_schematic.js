// Fusion baseboard schematic for intelligent balancing trolley
// Based on original baseboard + fusion additions:
//   - USART2 header (PA2/PA3) for Raspberry Pi
//   - Beeper on PA4
//   - Infrared sensor headers (PC13/PC14/PC15/PA7)
//   - Expansion headers

const LIB = "0819f05c4eef4c71ace90d822a990e87";
const log = (msg) => console.log(msg);

async function findDev(kw) {
  const r = await eda.lib_Device.search(kw, undefined, undefined, undefined, 5, 1);
  return r.length > 0 ? r[0] : null;
}

async function placeComp(dUuid, x, y, des, name, rot) {
  try {
    const c = await eda.sch_PrimitiveComponent.create(
      { uuid: dUuid, libraryUuid: LIB }, x, y, undefined, rot || 0
    );
    if (c) {
      await eda.sch_PrimitiveComponent.modify(c.primitiveId, {
        designator: des, name: name || ""
      });
    }
    return c;
  } catch (e) {
    log("Error placing " + des + ": " + e.message);
    return null;
  }
}

async function placePower(type, net, x, y, rot) {
  try {
    return await eda.sch_PrimitiveComponent.createNetFlag(type, net, x, y, rot || 0);
  } catch (e) { return null; }
}

async function placePort(dir, net, x, y, rot) {
  try {
    return await eda.sch_PrimitiveComponent.createNetPort(dir, net, x, y, rot || 0);
  } catch (e) { return null; }
}

async function drawWire(points, net) {
  try {
    return await eda.sch_PrimitiveWire.create(points, net);
  } catch (e) { return null; }
}

async function addText(x, y, text) {
  try {
    return await eda.sch_PrimitiveText.create(x, y, text);
  } catch (e) { return null; }
}

// ──────────────────────────────────────────────────
// MAIN
// ──────────────────────────────────────────────────
const result = { placed: [], errors: [] };

// Phase 1: Search components
log("=== Phase 1: Searching components ===");
const searches = {
  tb6612:    "TB6612FNG",
  mpu6050:   "MPU6050",
  cap_100nf: "100nF 0805",
  cap_220uf: "220uF 63V",
  sw_spdt:   "SW-SPDT",
  dc005:     "DC-005",
  h1x2:      "Pin Header 1x2",
  h1x4:      "Pin Header 1x4",
  h1x6:      "Pin Header 1x6",
  h1x8:      "Pin Header 1x8",
  h1x10:     "Pin Header 1x10",
  h1x12:     "Pin Header 1x12",
  h1x15:     "Pin Header 1x15",
  h2x15:     "Pin Header 2x15",
  r1k:       "1k 0805",
  r10k:      "10k 0805",
  r100:      "100R 0805",
  npn:       "S8050",
  buzzer:    "Buzzer",
  led_blue:  "LED 0805 Blue",
  led_red:   "LED 0805 Red",
};

const devs = {};
for (const [key, kw] of Object.entries(searches)) {
  const d = await findDev(kw);
  devs[key] = d ? d.uuid : null;
  log("  " + key + ": " + (devs[key] ? "OK" : "NOT FOUND"));
}

// Phase 2: Place components
log("=== Phase 2: Placing components ===");

// --- Power section (left side, y=100-200) ---
const powerComps = [
  ["J9",  devs.dc005,     80,  120, "DC-005",   0],
  ["SW1", devs.sw_spdt,   220, 120, "SW-SPDT",  0],
  ["C2",  devs.cap_220uf, 350, 120, "220uF/63V", 0],
  ["J6",  devs.h1x4,      500, 120, "Power_Out", 0],
  ["C3",  devs.cap_100nf, 620, 90,  "104",      0],
  ["C4",  devs.cap_100nf, 700, 90,  "104",      0],
  ["J5",  devs.h1x10,     830, 120, "12TO5",    0],
];

// --- MCU section (center, y=300-400) ---
const mcuComps = [
  ["J1_L", devs.h1x15, 250, 350, "Forest_S1_L", 0],
  ["J1_R", devs.h1x15, 450, 350, "Forest_S1_R", 0],
];

// --- Motor driver (right, y=300) ---
const motorComps = [
  ["tb1", devs.tb6612,   780, 350, "TB6612",   0],
  ["C1",  devs.cap_100nf, 780, 250, "104",      0],
];

// --- IMU section (bottom, y=550) ---
const imuComps = [
  ["MPU1", devs.h1x8,  400, 600, "MPU6050", 0],
];

// --- Left side connectors ---
const leftComps = [
  ["J8",  devs.h1x6,  80, 350, "WIFI_BT",  0],
  ["J3",  devs.h1x4,  80, 470, "OLED_SPI", 0],
  ["P1",  devs.h1x6,  80, 580, "Encoder",  0],
];

// --- Right side connectors (fusion additions) ---
const rightComps = [
  ["J10", devs.h1x4,  950, 250, "RPi_UART",  0],  // USART2 for RPi
  ["P2",  devs.h1x6,  950, 370, "Infrared",   0],  // Infrared sensors
  ["J4",  devs.h1x4,  950, 490, "SWD",        0],  // SWD debug
  ["P3",  devs.h1x12, 950, 600, "Expansion",  0],  // General expansion
];

// --- Beeper section ---
const beeperComps = [
  ["Q1", devs.npn,     620, 500, "S8050",    0],
  ["R10", devs.r1k,    620, 430, "1k",       0],
  ["BZ1", devs.buzzer, 700, 500, "Buzzer",   0],
  ["LED_B", devs.led_blue, 700, 430, "LED_B", 0],
];

// Place all
const allComps = [...powerComps, ...mcuComps, ...motorComps, ...imuComps,
                 ...leftComps, ...rightComps, ...beeperComps];

for (const [des, uuid, x, y, name, rot] of allComps) {
  if (!uuid) { result.errors.push("No UUID: " + des); continue; }
  const c = await placeComp(uuid, x, y, des, name, rot);
  if (c) result.placed.push(des);
  else result.errors.push("Failed: " + des);
}

// Phase 3: Power flags
log("=== Phase 3: Power flags ===");
await placePower("Power",  "12V", 100, 80, 0);
await placePower("Power",  "12V", 830, 80, 0);
await placePower("Power",  "5V",  650, 80, 0);
await placePower("Power",  "5V",  300, 310, 0);
await placePower("Power",  "3V3", 500, 310, 0);
await placePower("Ground", "GND", 100, 180, 0);
await placePower("Ground", "GND", 650, 180, 0);
await placePower("Ground", "GND", 450, 680, 0);
await placePower("Ground", "GND", 700, 560, 0);

// Phase 4: Net ports for key signals
log("=== Phase 4: Net ports ===");
await placePort("Output", "USART2_TX", 950, 230, 0);
await placePort("Input",  "USART2_RX", 950, 270, 0);
await placePort("Output", "BEEP",       620, 410, 0);
await placePort("Input",  "IR1",        950, 350, 0);
await placePort("Input",  "IR2",        950, 380, 0);
await placePort("Input",  "IR3",        950, 410, 0);
await placePort("Input",  "IR4",        950, 440, 0);

// Phase 5: Text annotations
log("=== Phase 5: Text annotations ===");
await addText(80, 330, "WIFI/蓝牙模块");
await addText(80, 450, "OLED SPI接口");
await addText(80, 560, "编码器接口");
await addText(250, 310, "Forest S1 STM32F103C8T6");
await addText(780, 310, "TB6612电机驱动");
await addText(400, 580, "MPU6050六轴IMU");
await addText(620, 480, "蜂鸣器+LED(PA4)");
await addText(950, 230, "树莓派UART(USART2)");
await addText(950, 350, "红外循迹(4路)");

result.totalPlaced = result.placed.length;
result.totalErrors = result.errors.length;
log("Done: " + result.totalPlaced + " placed, " + result.totalErrors + " errors");
return result;
