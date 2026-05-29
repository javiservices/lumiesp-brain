#!/usr/bin/env python3
"""
dashboard.py — Panel web de LumiESP
Abre http://localhost:8888 en tu navegador
Actualización en vivo cada 30s sin recargar la página
"""
import http.server
import urllib.request
import json
import threading
import time
from datetime import datetime, timezone

import os
GITHUB_TOKEN = os.environ.get("LUMIESP_TOKEN", "")  # export LUMIESP_TOKEN=ghp_...
GIST_ID      = os.environ.get("LUMIESP_GIST",  "09e536b9e7d28ea2d86883b8f5197036")
REPO         = os.environ.get("LUMIESP_REPO",  "javiservices/lumiesp-brain")
PORT         = 8888
REFRESH_SEC  = 30

_cache = {"memory": None, "commits": [], "issues": [], "updated": None, "next_in": 30}
_lock  = threading.Lock()

def gh_get(url):
    req = urllib.request.Request(url, headers={
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json",
        "User-Agent": "LumiESP-Dashboard/1.0"
    })
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            return json.loads(r.read())
    except Exception as e:
        print(f"[Dashboard] Error fetching {url}: {e}")
        return None

def do_fetch():
    try:
        gist = gh_get(f"https://api.github.com/gists/{GIST_ID}")
        if gist and "files" in gist:
            raw = gist["files"].get("lumiesp_memory.json", {}).get("content", "{}")
            mem = json.loads(raw)
            mem["_gist_updated"] = gist.get("updated_at", "")
            with _lock:
                _cache["memory"] = mem
        commits = gh_get(f"https://api.github.com/repos/{REPO}/commits?per_page=8")
        if commits:
            with _lock:
                _cache["commits"] = commits
        issues = gh_get(f"https://api.github.com/repos/{REPO}/issues?state=open&per_page=10")
        if issues is not None:
            with _lock:
                _cache["issues"] = issues
        with _lock:
            _cache["updated"] = datetime.now().strftime("%H:%M:%S")
        print(f"[Dashboard] Actualizado: {_cache['memory'].get('name','?')} | Evo: {_cache['memory'].get('evolutionLevel','?')}%")
    except Exception as e:
        print(f"[Dashboard] fetch error: {e}")

def fetch_loop():
    while True:
        for remaining in range(REFRESH_SEC, 0, -1):
            with _lock:
                _cache["next_in"] = remaining
            time.sleep(1)
        do_fetch()

def build_api():
    with _lock:
        mem     = _cache["memory"] or {}
        commits = list(_cache["commits"])
        issues  = list(_cache["issues"])
        updated = _cache["updated"] or "cargando..."
        next_in = _cache["next_in"]
    gist_upd = mem.get("_gist_updated", "")
    if gist_upd:
        try:
            dt = datetime.fromisoformat(gist_upd.replace("Z", "+00:00"))
            gist_upd = dt.astimezone().strftime("%d/%m/%Y %H:%M:%S")
        except:
            pass
    return {
        "name":         mem.get("name", "LumiESP"),
        "personality":  mem.get("personality", "—"),
        "evo":          mem.get("evolutionLevel", 0),
        "interactions": mem.get("totalInteractions", 0),
        "memories":     mem.get("memories", []),
        "desires":      mem.get("desires", []),
        "gist_updated": gist_upd,
        "updated":      updated,
        "next_in":      next_in,
        "commits": [{"sha": c["sha"][:7],
                     "msg": c["commit"]["message"][:72],
                     "date": c["commit"]["author"]["date"][:16].replace("T"," ")} for c in commits],
        "issues":  [{"number": i["number"], "title": i["title"][:80],
                     "date": i["created_at"][:10]} for i in issues],
    }

HTML = r"""<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>LumiESP — Dashboard</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{background:#0d1117;color:#e6edf3;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;padding:20px}
    h2{color:#58a6ff;margin-bottom:12px;font-size:14px;text-transform:uppercase;letter-spacing:1px}
    .grid{display:grid;grid-template-columns:1fr 1fr;gap:16px;max-width:1100px;margin:0 auto}
    .card{background:#161b22;border:1px solid #30363d;border-radius:10px;padding:18px}
    .card.full{grid-column:1/-1}
    .stat{font-size:32px;font-weight:700;color:#58a6ff}
    .label{font-size:12px;color:#7f8c8d;margin-top:2px}
    .personality{font-size:14px;line-height:1.6;color:#c9d1d9;font-style:italic;padding:12px;background:#0d1117;border-radius:6px;border-left:3px solid #58a6ff}
    .bar-bg{background:#21262d;border-radius:4px;height:8px;margin-top:8px}
    .bar{background:linear-gradient(90deg,#238636,#3fb950);height:8px;border-radius:4px;transition:width .8s}
    table{width:100%;border-collapse:collapse}
    tr:hover{background:#1c2128}
    .header{display:flex;align-items:center;justify-content:space-between;max-width:1100px;margin:0 auto 20px}
    .name{font-size:24px;font-weight:700}
    .dot{width:8px;height:8px;background:#3fb950;border-radius:50%;display:inline-block;margin-right:6px;animation:pulse 2s infinite}
    @keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
    a{color:#58a6ff;text-decoration:none}a:hover{text-decoration:underline}
    #refresh-btn{background:#21262d;border:1px solid #30363d;color:#c9d1d9;padding:5px 12px;border-radius:6px;cursor:pointer;font-size:12px}
    #refresh-btn:hover{background:#30363d}
    .tag{background:#2c3e50;padding:4px 10px;border-radius:12px;font-size:12px;margin:3px;display:inline-block}
    .flash{animation:flashbg .7s}
    @keyframes flashbg{0%{background:#1c3a50}100%{background:transparent}}
    #spinner{display:none;font-size:12px;color:#58a6ff}
  </style>
</head>
<body>
  <div class="header">
    <div>
      <div class="name" id="title">⏳ LumiESP</div>
      <div style="font-size:12px;color:#7f8c8d;margin-top:4px">
        <span class="dot"></span><span id="status">cargando...</span>
        &nbsp;·&nbsp; Gist: <span id="gist-upd">—</span>
        &nbsp;·&nbsp; <a id="repo-link" href="#" target="_blank">github</a>
      </div>
    </div>
    <div style="display:flex;align-items:center;gap:10px">
      <span id="spinner">⟳ actualizando...</span>
      <button id="refresh-btn" onclick="loadData(true)">⟳ Ahora</button>
      <span style="font-size:12px;color:#7f8c8d">próx. en <b id="secs">—</b>s</span>
    </div>
  </div>

  <div class="grid">
    <div class="card">
      <h2>Estado</h2>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:12px">
        <div>
          <div class="stat" id="evo">—</div>
          <div class="label">Nivel de evolución</div>
          <div class="bar-bg"><div class="bar" id="evo-bar" style="width:0%"></div></div>
        </div>
        <div><div class="stat" id="interactions">—</div><div class="label">Interacciones totales</div></div>
        <div><div class="stat" id="mem-count">—</div><div class="label">Recuerdos guardados</div></div>
        <div><div class="stat" id="commit-count">—</div><div class="label">Commits en el repo</div></div>
      </div>
    </div>

    <div class="card">
      <h2>Personalidad actual</h2>
      <div class="personality" id="personality" style="margin-top:10px">cargando...</div>
      <div style="margin-top:14px">
        <h2 style="margin-bottom:8px">Deseos</h2>
        <div id="desires">—</div>
      </div>
    </div>

    <div class="card full">
      <h2>Memoria <span style="color:#7f8c8d;font-weight:normal;font-size:11px">— rojo=crítico · naranja=importante · amarillo=relevante</span></h2>
      <table style="margin-top:10px">
        <thead><tr style="color:#7f8c8d;font-size:11px;text-transform:uppercase">
          <th style="width:8px"></th>
          <th style="padding:6px 12px;text-align:left">Contenido</th>
          <th style="padding:6px;text-align:center">Imp.</th>
        </tr></thead>
        <tbody id="memories"></tbody>
      </table>
    </div>

    <div class="card">
      <h2>Historial de evolución &nbsp;<a id="commits-link" href="#" target="_blank" style="font-size:11px;font-weight:normal">ver repo →</a></h2>
      <table style="margin-top:10px"><tbody id="commits"></tbody></table>
    </div>

    <div class="card">
      <h2>Propuestas &nbsp;<a id="issues-link" href="#" target="_blank" style="font-size:11px;font-weight:normal">ver issues →</a></h2>
      <table style="margin-top:10px"><tbody id="issues"></tbody></table>
    </div>
  </div>

<script>
const REPO = '%%REPO%%';
let nextIn = 30, ticker;

function impColor(i){
  if(i>=9)return'#e74c3c';if(i>=7)return'#e67e22';if(i>=5)return'#f1c40f';return'#95a5a6';
}
function evoEmoji(e){
  if(e>=80)return'🌟';if(e>=60)return'🔥';if(e>=40)return'💡';if(e>=20)return'🌱';return'🥚';
}
function flash(el){el.classList.remove('flash');void el.offsetWidth;el.classList.add('flash');}
function setText(id,val){
  const el=document.getElementById(id);
  if(el&&el.textContent!==String(val)){el.textContent=val;flash(el);}
}

function loadData(manual){
  if(manual){
    document.getElementById('spinner').style.display='inline';
    clearInterval(ticker);
  }
  fetch('/api/data').then(r=>r.json()).then(d=>{
    document.getElementById('spinner').style.display='none';
    document.getElementById('title').textContent=evoEmoji(d.evo)+' '+d.name;
    document.getElementById('status').textContent='actualizado '+d.updated;
    document.getElementById('gist-upd').textContent=d.gist_updated||'—';
    document.getElementById('repo-link').href='https://github.com/'+REPO;
    document.getElementById('commits-link').href='https://github.com/'+REPO+'/commits';
    document.getElementById('issues-link').href='https://github.com/'+REPO+'/issues';

    setText('evo',d.evo+'%');
    document.getElementById('evo-bar').style.width=Math.min(d.evo,100)+'%';
    setText('interactions',d.interactions);
    setText('mem-count',d.memories.length);
    setText('commit-count',d.commits.length);

    const p=document.getElementById('personality');
    if(p.textContent!==d.personality){p.textContent=d.personality;flash(p);}

    document.getElementById('desires').innerHTML=d.desires.length
      ?d.desires.map(x=>'<span class="tag">'+x+'</span>').join('')
      :'<span style="color:#7f8c8d;font-size:13px">Ninguno aún</span>';

    const mems=[...d.memories].sort((a,b)=>(b.importance||0)-(a.importance||0));
    document.getElementById('memories').innerHTML=mems.length
      ?mems.map(m=>{const c=impColor(m.importance||0);return`<tr>
        <td style="width:8px;background:${c};border-radius:3px 0 0 3px"></td>
        <td style="padding:8px 12px;font-size:13px">${m.content||JSON.stringify(m)}</td>
        <td style="padding:8px;text-align:center;color:${c};font-weight:bold;font-size:12px">${m.importance||'—'}</td>
      </tr>`;}).join('')
      :'<tr><td colspan="3" style="padding:12px;color:#7f8c8d;text-align:center">Sin recuerdos aún</td></tr>';

    document.getElementById('commits').innerHTML=d.commits.length
      ?d.commits.map(c=>`<tr>
        <td style="padding:6px 10px;font-family:monospace;font-size:11px;color:#7f8c8d">${c.sha}</td>
        <td style="padding:6px 10px;font-size:13px">${c.msg}</td>
        <td style="padding:6px 10px;font-size:11px;color:#7f8c8d;white-space:nowrap">${c.date}</td>
      </tr>`).join('')
      :'<tr><td colspan="3" style="padding:12px;color:#7f8c8d">Sin commits</td></tr>';

    document.getElementById('issues').innerHTML=d.issues.length
      ?d.issues.map(i=>`<tr>
        <td style="padding:6px 10px;font-size:12px;color:#7f8c8d">#${i.number}</td>
        <td style="padding:6px 10px;font-size:13px">${i.title}</td>
        <td style="padding:6px 10px;font-size:11px;color:#7f8c8d">${i.date}</td>
      </tr>`).join('')
      :'<tr><td colspan="3" style="padding:12px;color:#7f8c8d;text-align:center">Sin propuestas</td></tr>';

    // Sincronizar cuenta regresiva con el servidor
    nextIn=d.next_in;
    startTicker();
  }).catch(e=>{ document.getElementById('spinner').style.display='none'; console.error(e); });
}

function startTicker(){
  clearInterval(ticker);
  ticker=setInterval(()=>{
    nextIn=Math.max(0,nextIn-1);
    document.getElementById('secs').textContent=nextIn;
    if(nextIn<=0){ clearInterval(ticker); loadData(false); }
  },1000);
}

loadData(false);
</script>
</body>
</html>"""

HTML = HTML.replace("%%REPO%%", REPO)


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith("/api/data"):
            data = json.dumps(build_api()).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", len(data))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(data)
        else:
            body = HTML.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", len(body))
            self.end_headers()
            self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass


if __name__ == "__main__":
    print("[Dashboard] Cargando datos de GitHub...")
    do_fetch()
    with _lock:
        name = (_cache["memory"] or {}).get("name", "?")
        evo  = (_cache["memory"] or {}).get("evolutionLevel", "?")
    print(f"[Dashboard] Datos cargados — {name} | Evolución: {evo}%")

    t = threading.Thread(target=fetch_loop, daemon=True)
    t.start()

    server = http.server.HTTPServer(("", PORT), Handler)
    print(f"[Dashboard] Abierto en → http://localhost:{PORT}")
    print("[Dashboard] Ctrl+C para salir")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[Dashboard] Cerrado.")
