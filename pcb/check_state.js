const WS = require('ws');
const ws = new WS('ws://localhost:49620');

let id = 0;
function rpc(method, params) {
  return new Promise((res, rej) => {
    const rid = ++id;
    const timer = setTimeout(() => rej(new Error('timeout')), 15000);
    ws.on('message', function handler(d) {
      const m = JSON.parse(d);
      if (m.id === rid) { clearTimeout(timer); ws.off('message', handler); res(m); }
    });
    ws.send(JSON.stringify({ jsonrpc: '2.0', id: rid, method, params: params || [] }));
  });
}

async function run() {
  try {
    // Get current doc info
    const docInfo = await rpc('invoke', ['getCurrentDocumentInfo', []]);
    console.log('DOC:', JSON.stringify(docInfo.result));

    // List all components on the schematic
    const comps = await rpc('invoke', ['getAllPrimitiveComponents', []]);
    console.log('COMPS:', JSON.stringify(comps.result));

  } catch(e) {
    console.log('ERROR:', e.message);
  }
  ws.close();
}

ws.on('open', run);
