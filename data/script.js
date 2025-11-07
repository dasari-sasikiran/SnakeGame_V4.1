/*
Improved WebSocket client for ESP32 Gaming Console.
- Uses the safer gateway construction that works behind Cloudflared.
- Preserves your command map and voice recognition.
*/

(function () {
  // --- Build the correct gateway ---
  const host = window.location.host;                // includes :port if present
  const scheme = (window.location.protocol === 'https:') ? 'wss' : 'ws';
  const gateway = `${scheme}://${host}/ws`;         // identical to working reference
  const RECONNECT_MS = 1500;

  // --- Command Map ---
  const CMD = {
    UP_HIGH:'UP_HIGH', UP_LOW:'UP_LOW',
    DOWN_HIGH:'DOWN_HIGH', DOWN_LOW:'DOWN_LOW',
    LEFT_HIGH:'LEFT_HIGH', LEFT_LOW:'LEFT_LOW',
    RIGHT_HIGH:'RIGHT_HIGH', RIGHT_LOW:'RIGHT_LOW',
    PAUSE:'PAUSE_PLAY', RESTART:'RESTART', MUTE:'MUTE'
  };

  // --- DOM Elements ---
  const wsIndicator   = document.getElementById('ws-indicator');
  const lastText      = document.getElementById('last-text');
  const micBtn        = document.getElementById('micBtn') || document.getElementById('btn-voice');
  const micStatus     = document.getElementById('micStatus');
  const recognizedText= document.getElementById('recognizedText');
  const logArea       = document.getElementById('logArea');

  // Logger to page or console
  function uiLog(msg){
    if(logArea){
      const p=document.createElement('div');
      p.textContent=`[${new Date().toLocaleTimeString()}] ${msg}`;
      p.style.margin='4px 0';
      logArea.prepend(p);
    } else console.log(msg);
  }

  let websocket=null, reconnectTimer=null;

  function setStatus(connected){
    if(wsIndicator){
      wsIndicator.textContent=connected?'Connected':'Disconnected';
      wsIndicator.style.color=connected?'lightgreen':'#f43f5e';
    } else uiLog('WS '+(connected?'Connected':'Disconnected'));
  }

  function initWebSocket(){
    console.log('Trying WebSocket:', gateway);
    websocket = new WebSocket(gateway);

    websocket.onopen = ()=>{
      uiLog('WebSocket opened');
      setStatus(true);
      if(reconnectTimer){clearTimeout(reconnectTimer); reconnectTimer=null;}
      websocket.send('states');
    };

    websocket.onclose = ()=>{
      uiLog('WebSocket closed');
      setStatus(false);
      reconnectTimer = setTimeout(initWebSocket, RECONNECT_MS);
    };

    websocket.onerror = (e)=>{
      console.error('WS error', e);
      uiLog('WebSocket error');
    };

    websocket.onmessage = (ev)=>{
      console.log('WS RX', ev.data);
      if(lastText) lastText.textContent = ev.data;
      uiLog('RX: '+ev.data);
    };
  }

  // Utility to send commands
  function sendCommand(cmd){
    if(!websocket || websocket.readyState!==WebSocket.OPEN){
      uiLog('WS not open, cannot send: '+cmd);
      return;
    }
    websocket.send(cmd);
    if(lastText) lastText.textContent = cmd;
    uiLog('TX: '+cmd);
  }

  // Attach directional button handlers
  function attachPressRelease(id, high, low){
    const el=document.getElementById(id); if(!el) return;
    el.addEventListener('mousedown',e=>{e.preventDefault();sendCommand(high);});
    el.addEventListener('mouseup',e=>{e.preventDefault();sendCommand(low);});
    el.addEventListener('mouseleave',e=>{e.preventDefault();sendCommand(low);});
    el.addEventListener('touchstart',e=>{e.preventDefault();sendCommand(high);},{passive:false});
    el.addEventListener('touchend',e=>{e.preventDefault();sendCommand(low);});
    el.addEventListener('touchcancel',e=>{e.preventDefault();sendCommand(low);});
  }

  function attachToggle(id, cmd){
    const el=document.getElementById(id); if(!el) return;
    el.addEventListener('click',e=>{e.preventDefault();sendCommand(cmd);});
  }

  attachPressRelease('btn-up',CMD.UP_HIGH,CMD.UP_LOW);
  attachPressRelease('btn-down',CMD.DOWN_HIGH,CMD.DOWN_LOW);
  attachPressRelease('btn-left',CMD.LEFT_HIGH,CMD.LEFT_LOW);
  attachPressRelease('btn-right',CMD.RIGHT_HIGH,CMD.RIGHT_LOW);
  attachToggle('btn-pause',CMD.PAUSE);
  attachToggle('btn-restart',CMD.RESTART);
  attachToggle('btn-mute',CMD.MUTE);

  // Keyboard shortcuts
  function impulse(h,l){sendCommand(h);setTimeout(()=>sendCommand(l),120);}
  window.addEventListener('keydown',e=>{
    switch(e.key){
      case 'ArrowUp':case 'w':case 'W': impulse(CMD.UP_HIGH,CMD.UP_LOW); break;
      case 'ArrowDown':case 's':case 'S': impulse(CMD.DOWN_HIGH,CMD.DOWN_LOW); break;
      case 'ArrowLeft':case 'a':case 'A': impulse(CMD.LEFT_HIGH,CMD.LEFT_LOW); break;
      case 'ArrowRight':case 'd':case 'D': impulse(CMD.RIGHT_HIGH,CMD.RIGHT_LOW); break;
      case ' ': sendCommand(CMD.PAUSE); break;
      case 'm':case 'M': sendCommand(CMD.MUTE); break;
      case 'r':case 'R': sendCommand(CMD.RESTART); break;
    }
  });

  // ---- Voice Recognition ----
  const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition || null;
  let recognizer=null, listening=false, lastCmd=null,lastTs=0;

  function mapTranscriptToCommand(t){
    t=t.toUpperCase();
    if(t.includes('RESTART')) return CMD.RESTART;
    if(t.includes('STOP')||t.includes('PLAY')) return CMD.PAUSE;
    if(t.includes('MUTE')||t.includes('UNMUTE')) return CMD.MUTE;

    if(/\bUp\b/.test(t)) return CMD.UP_HIGH;
    if(/\bDown\b/.test(t)) return CMD.DOWN_HIGH;
    if(/\bLEFT\b/.test(t)) return CMD.LEFT_HIGH;
    if(/\bRIGHT\b/.test(t)) return CMD.RIGHT_HIGH;
    // if(/\bUPSIDE\b/.test(t)) return CMD.UP_HIGH;
    // if(/\bDOWNWARDS\b/.test(t)) return CMD.DOWN_HIGH;
    // if(/\bLEFT\b/.test(t)) return CMD.LEFT_HIGH;
    // if(/\bRIGHT\b/.test(t)) return CMD.RIGHT_HIGH;
    return null;
  }

  if(!SpeechRecognition){
    if(micStatus) micStatus.textContent='Unsupported';
    if(micBtn) micBtn.disabled=true;
    uiLog('SpeechRecognition not supported');
  } else {
    recognizer = new SpeechRecognition();
    recognizer.continuous=true;
    recognizer.interimResults=false;
    recognizer.lang='en-US';

    recognizer.onresult = e=>{
      const transcript=e.results[e.results.length-1][0].transcript.trim();
      if(recognizedText) recognizedText.textContent=transcript;
      uiLog('Recognized: '+transcript);
      try{websocket.send('VOICE:'+transcript);}catch{}
      const cmd=mapTranscriptToCommand(transcript);
      const now=Date.now();
      if(cmd && (cmd!==lastCmd || now-lastTs>400)){
        lastCmd=cmd; lastTs=now;
        sendCommand(cmd);
      }
    };
    recognizer.onstart=()=>{
      listening=true;
      if(micBtn){micBtn.textContent='🎙 Listening';micBtn.classList.add('recording');}
      if(micStatus) micStatus.textContent='Listening';
    };
    recognizer.onend=()=>{
      listening=false;
      if(micBtn){micBtn.textContent='🎤 Start';micBtn.classList.remove('recording');}
      if(micStatus) micStatus.textContent='Stopped';
      if(micBtn && micBtn.dataset.keep==='true') setTimeout(()=>recognizer.start(),300);
    };
    if(micBtn){
      micBtn.addEventListener('click',()=>{
        if(!listening){micBtn.dataset.keep='true';recognizer.start();}
        else{micBtn.dataset.keep='false';recognizer.stop();}
      });
    }
  }

  // --- Start connection ---
  window.addEventListener('load', initWebSocket);
  window._espConsole={sendCommand};

})();
