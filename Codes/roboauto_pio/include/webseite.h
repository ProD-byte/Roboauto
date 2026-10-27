#pragma once
// Die Steuer-Webseite, die das Auto im Browser anzeigt (HTML + CSS + JavaScript in einem).
// R"==(...)==" ist ein "Raw String": der Text steht 1:1 so im Programm.

// WLAN-Einrichtungsseite: Kopf und Fuss (der Inhalt dazwischen wird im Programm zusammengebaut)
const char EINRICHTUNG_KOPF[] PROGMEM = R"==(<!doctype html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0d0f13">
<title>Roboauto &ndash; WLAN einrichten</title>
<style>
  :root{--bg:#0d0f13;--karte:#161a20;--karte2:#1c2129;--rand:#262c35;--rand2:#323a45;--text:#eef1f5;--leise:#8b95a3;
        --akzent:#4f9cff;--akzent2:#7c6cff;--gut:#3ccf6e;--rot:#ef5350}
  *{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
  body{margin:0;min-height:100vh;color:var(--text);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
       background:radial-gradient(700px 420px at 0% -10%,#1a2440 0%,transparent 60%),radial-gradient(600px 400px at 110% 0%,#251c40 0%,transparent 55%),var(--bg);
       padding:max(18px,env(safe-area-inset-top)) 16px 28px}
  .rahmen{max-width:460px;margin:0 auto;display:flex;flex-direction:column;gap:16px}
  .kopf{display:flex;align-items:center;gap:12px;padding:6px 2px}
  .kopf b{font-size:20px;display:block}
  .kopf small{color:var(--leise);font-size:13px}
  .karte{background:linear-gradient(180deg,var(--karte2),var(--karte));border:1px solid var(--rand);border-radius:20px;padding:18px;box-shadow:0 14px 40px -20px #000}
  h2{margin:0;font-size:17px}
  a{color:var(--akzent);text-decoration:none}
  .leise{color:var(--leise);font-size:14px}
  .hinweis{color:var(--leise);font-size:13px;text-align:center;margin:0}
  /* Schritte */
  .schritte{display:flex;gap:6px;margin-bottom:16px}
  .schritte span{flex:1;text-align:center;font-size:12px;color:var(--leise);padding-top:8px;border-top:3px solid var(--rand2);transition:all .3s}
  .schritte span.an{color:var(--text);border-color:var(--akzent)}
  .schritte span.fertig{color:var(--gut);border-color:var(--gut)}
  .zeile{display:flex;align-items:center;justify-content:space-between;margin-bottom:10px}
  .neu{font-size:14px;padding:6px 12px;border-radius:999px;border:1px solid var(--rand2);background:var(--karte2);color:var(--text)}
  /* Netzwerkliste */
  .netze{display:flex;flex-direction:column;gap:8px;max-height:46vh;overflow:auto;margin:0 -4px;padding:2px 4px}
  .netz{display:flex;align-items:center;gap:12px;padding:13px 14px;border-radius:14px;background:#12161c;border:1px solid var(--rand);cursor:pointer;transition:border-color .15s,background .15s,box-shadow .2s}
  .netz input{position:absolute;opacity:0;pointer-events:none}
  .netz .name{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-weight:550}
  .netz svg{flex:none}
  .netz.gewaehlt{border-color:var(--akzent);background:#162540;box-shadow:0 0 0 1px var(--akzent) inset,0 0 20px -6px var(--akzent)}
  .netz .haken{width:22px;height:22px;border-radius:50%;border:2px solid var(--rand2);flex:none;display:grid;place-items:center;font-size:13px;color:transparent;transition:all .15s}
  .netz.gewaehlt .haken{background:var(--akzent);border-color:var(--akzent);color:#fff}
  .marke{font-size:11px;font-weight:600;padding:3px 8px;border-radius:999px;background:#1b2a44;color:#9cc4ff;border:1px solid #2d4a7a;flex:none}
  .marke.gruen{background:#12301f;color:#7fe0a3;border-color:#2f8f52}
  .gespeichert{display:flex;flex-direction:column;gap:8px}
  .gsp{display:flex;align-items:center;gap:10px;padding:11px 12px 11px 14px;border-radius:14px;background:#12161c;border:1px solid var(--rand)}
  .gsp .name{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
  .weg{width:34px;height:34px;border-radius:10px;display:grid;place-items:center;color:var(--leise);background:#1c2129;border:1px solid var(--rand2);flex:none;font-size:14px}
  .weg:hover{color:#fff;background:#4a1c1c;border-color:#a33a3a}
  .lade{height:50px;border-radius:14px;background:linear-gradient(90deg,#14181e 0%,#1e242d 50%,#14181e 100%);background-size:200% 100%;animation:schimmer 1.2s infinite}
  @keyframes schimmer{to{background-position:-200% 0}}
  details.hand{margin-top:12px}
  details.hand summary{cursor:pointer;color:var(--leise);font-size:14px;list-style:none}
  details.hand summary::before{content:'+ '}
  details[open].hand summary::before{content:'\2212 '}
  /* Eingaben */
  label.feld{display:block;margin:18px 0 8px;font-size:14px;color:var(--leise)}
  input.text{width:100%;padding:14px 16px;border-radius:14px;border:1px solid var(--rand2);background:#12161c;color:var(--text);font-size:16px;outline:none;transition:border-color .15s,box-shadow .15s}
  input.text:focus{border-color:var(--akzent);box-shadow:0 0 0 3px #4f9cff33}
  details.hand input.text{margin-top:10px}
  .pwfeld{position:relative}
  .pwfeld input{padding-right:54px}
  .auge{position:absolute;right:6px;top:6px;width:42px;height:42px;border:0;border-radius:10px;background:transparent;color:var(--leise);font-size:20px;cursor:pointer}
  .haupt{width:100%;margin-top:20px;height:54px;border:0;border-radius:16px;color:#fff;font:650 17px system-ui,sans-serif;cursor:pointer;
         background:linear-gradient(90deg,var(--akzent),var(--akzent2));box-shadow:0 10px 26px -10px #4f9cff;transition:opacity .2s,transform .08s}
  .haupt:active{transform:scale(.98)}
  .haupt:disabled{opacity:.35;box-shadow:none;cursor:default}
  .meldung{display:flex;gap:10px;align-items:flex-start;padding:12px 14px;border-radius:14px;font-size:14px;margin-bottom:14px}
  .meldung.ok{background:#12301f;border:1px solid #2f8f52}
  .meldung.fehler{background:#351818;border:1px solid #a33a3a}
  /* Erfolg */
  .erfolg{text-align:center;display:flex;flex-direction:column;align-items:center;gap:12px}
  .kreis{width:84px;height:84px;border-radius:50%;display:grid;place-items:center;background:radial-gradient(circle,#1d4d33,#12301f);border:2px solid var(--gut);box-shadow:0 0 30px -6px var(--gut);animation:auf .5s cubic-bezier(.3,1.6,.5,1)}
  .kreis.rot{background:radial-gradient(circle,#4d1d1d,#301212);border-color:var(--rot);box-shadow:0 0 30px -6px var(--rot)}
  @keyframes auf{from{transform:scale(.3);opacity:0}}
  .adresse{font:600 18px ui-monospace,monospace;background:#12161c;border:1px solid var(--rand2);border-radius:12px;padding:10px 16px;color:var(--akzent)}
  .liste{text-align:left;margin:4px 0 0;padding-left:20px;color:var(--leise);font-size:14px}
  .liste li{margin:4px 0}
</style></head><body>
<div class="rahmen">
<div class="kopf"><svg viewBox="-66 -18 132 60" width="64" height="30" aria-hidden="true"><rect x="-40" y="-14" width="80" height="4" fill="#e6e6e6" stroke="#222" stroke-width=".6"/><rect x="-40" y="-10" width="80" height="12" fill="#ffd54f" stroke="#222" stroke-width=".6"/><rect x="-40" y="2" width="80" height="17" fill="#404040" stroke="#222" stroke-width=".6"/><circle cx="-33" cy="10" r="27" fill="#404040" stroke="#111" stroke-width=".8"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(0 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(20 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(40 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(60 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(80 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(100 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(120 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(140 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(160 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(180 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(200 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(220 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(240 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(260 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(280 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(300 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(320 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(340 -33 10)"/><circle cx="-33" cy="10" r="17" fill="#383838" stroke="#1e1e1e" stroke-width="1.2"/><circle cx="-33" cy="10" r="14" fill="none" stroke="#2a2a2a" stroke-width="1"/><line x1="-29.5" y1="12.0" x2="-20.9" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="-33.0" y1="14.0" x2="-33.0" y2="24.0" stroke="#222" stroke-width="1.6"/><line x1="-36.5" y1="12.0" x2="-45.1" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="-36.5" y1="8.0" x2="-45.1" y2="3.0" stroke="#222" stroke-width="1.6"/><line x1="-33.0" y1="6.0" x2="-33.0" y2="-4.0" stroke="#222" stroke-width="1.6"/><line x1="-29.5" y1="8.0" x2="-20.9" y2="3.0" stroke="#222" stroke-width="1.6"/><circle cx="-33" cy="10" r="4.5" fill="#c8cde0" stroke="#222" stroke-width=".8"/><circle cx="33" cy="10" r="27" fill="#404040" stroke="#111" stroke-width=".8"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(0 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(20 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(40 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(60 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(80 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(100 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(120 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(140 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(160 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(180 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(200 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(220 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(240 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(260 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(280 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(300 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(320 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(340 33 10)"/><circle cx="33" cy="10" r="17" fill="#383838" stroke="#1e1e1e" stroke-width="1.2"/><circle cx="33" cy="10" r="14" fill="none" stroke="#2a2a2a" stroke-width="1"/><line x1="36.5" y1="12.0" x2="45.1" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="33.0" y1="14.0" x2="33.0" y2="24.0" stroke="#222" stroke-width="1.6"/><line x1="29.5" y1="12.0" x2="20.9" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="29.5" y1="8.0" x2="20.9" y2="3.0" stroke="#222" stroke-width="1.6"/><line x1="33.0" y1="6.0" x2="33.0" y2="-4.0" stroke="#222" stroke-width="1.6"/><line x1="36.5" y1="8.0" x2="45.1" y2="3.0" stroke="#222" stroke-width="1.6"/><circle cx="33" cy="10" r="4.5" fill="#c8cde0" stroke="#222" stroke-width=".8"/></svg><div><b>Roboauto</b><small data-en="Wi-Fi setup">WLAN einrichten</small></div></div>
)==";

const char EINRICHTUNG_FUSS[] PROGMEM = R"==(</div>
<script>
(function(){
  // Englisch, wenn das Handy nicht auf Deutsch steht
  if (!/^de/i.test(navigator.language || 'de')){
    document.documentElement.lang = 'en';
    document.querySelectorAll('[data-en]').forEach(e => e.innerHTML = e.dataset.en);
    document.querySelectorAll('[data-en-ph]').forEach(e => e.placeholder = e.dataset.enPh);
  }
  const netze = document.querySelectorAll('.netz'), pw = document.getElementById('pw'),
        hand = document.getElementById('hand'), los = document.getElementById('los'), schritte = document.querySelectorAll('.schritte span');
  function pruefe(){
    const gewaehlt = document.querySelector('.netz input:checked');
    netze.forEach(n => n.classList.toggle('gewaehlt', n.querySelector('input').checked));
    const ok = !!gewaehlt || (hand && hand.value.trim().length > 0);
    if (los) los.disabled = !ok;
    if (los && schritte.length){ schritte[0].className = ok ? 'fertig' : 'an'; schritte[1].className = ok ? 'an' : ''; }
    if (pw && gewaehlt){
      const offen = gewaehlt.dataset.offen === '1';
      const bekannt = gewaehlt.dataset.bekannt === '1', en = document.documentElement.lang === 'en';
      if (bekannt && !offen){ pw.placeholder = en ? 'leave empty = use saved password' : 'leer lassen = gespeichertes Passwort'; return; }
      pw.placeholder = offen ? (document.documentElement.lang === 'en' ? 'open network - no password needed' : 'offenes Netz – kein Passwort nötig')
                             : (document.documentElement.lang === 'en' ? 'Wi-Fi password' : 'WLAN-Passwort');
    }
  }
  netze.forEach(n => n.querySelector('input').addEventListener('change', () => { if (hand) hand.value = ''; pruefe(); pw && pw.focus(); }));
  if (hand) hand.addEventListener('input', () => { if (hand.value) document.querySelectorAll('.netz input').forEach(i => i.checked = false); pruefe(); });
  const auge = document.getElementById('auge');
  if (auge) auge.addEventListener('click', () => { pw.type = pw.type === 'password' ? 'text' : 'password'; auge.textContent = pw.type === 'password' ? '\u{1F441}' : '\u{1F648}'; });
  if (los) los.form.addEventListener('submit', () => { los.disabled = true; los.textContent = document.documentElement.lang === 'en' ? 'Saving…' : 'Speichert…'; });
  document.querySelectorAll('.weg').forEach(a => a.addEventListener('click', e => {
    const en = document.documentElement.lang === 'en';
    if (!confirm((en ? 'Forget "' : '"') + a.dataset.name + (en ? '"?' : '" vergessen?'))) e.preventDefault();
  }));
  pruefe();
  if (document.getElementById('sucht')) setTimeout(() => location.reload(), 2500);   // Suche laeuft noch
})();
</script>
</body></html>
)==";

const char WEBSEITE[] PROGMEM = R"==(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Roboauto</title>
<style>
  :root{--bg:#0d0f13;--karte:#161a20;--karte2:#1c2129;--rand:#262c35;--rand2:#323a45;--text:#eef1f5;--leise:#8b95a3;
        --akzent:#4f9cff;--akzent2:#7c6cff;--gut:#3ccf6e;--warn:#f0a030;--rot:#ef5350;--gelb:#e6b800;--r:18px}
  *{box-sizing:border-box}
  [hidden]{display:none!important}
  html{scroll-behavior:smooth}
  body{margin:0;min-height:100vh;color:var(--text);font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;user-select:none;-webkit-user-select:none;
       background:radial-gradient(1200px 600px at 10% -10%,#1a2440 0%,transparent 60%),radial-gradient(900px 500px at 110% 10%,#251c40 0%,transparent 55%),var(--bg);background-attachment:fixed}
  a{color:var(--akzent);text-decoration:none}
  .leise{color:var(--leise);font-size:13px}
  /* ---- Kopfleiste ---- */
  .kopf{position:sticky;top:0;z-index:10;display:flex;align-items:center;gap:12px;flex-wrap:wrap;padding:10px max(16px,calc(50% - 560px));
        background:rgba(13,15,19,.72);backdrop-filter:blur(14px);-webkit-backdrop-filter:blur(14px);border-bottom:1px solid var(--rand)}
  .marke{display:flex;align-items:center;gap:10px;font-weight:700;font-size:19px;letter-spacing:.2px;margin-right:auto}
  .marke small{font-weight:500;font-size:11px;color:var(--leise);border:1px solid var(--rand2);border-radius:20px;padding:2px 8px}
  .pillen{display:flex;gap:8px;flex-wrap:wrap}
  .pille{display:flex;align-items:center;gap:7px;background:var(--karte);border:1px solid var(--rand);border-radius:999px;padding:6px 12px;font-size:13px;white-space:nowrap}
  .pille b{font-weight:650}
  .pille .t{color:var(--leise)}
  #punkt{width:9px;height:9px;border-radius:50%;background:var(--rot);box-shadow:0 0 8px currentColor;transition:background .3s}
  .akkuleiste{width:34px;height:8px;border-radius:4px;background:#2a3039;overflow:hidden}
  #akkubalken{height:100%;width:0;background:var(--gut);transition:width .5s,background .5s}
  #b-sprache{height:34px;width:auto;padding:0 12px;border-radius:999px;flex-direction:row;gap:6px;font-size:13px}
  /* ---- Seite ---- */
  section,details.einst{scroll-margin-top:110px}
  main{max-width:1120px;margin:0 auto;padding:18px 16px 40px;display:flex;flex-direction:column;gap:26px}
  h2{margin:0 0 10px;font-size:12px;font-weight:700;letter-spacing:1.4px;text-transform:uppercase;color:var(--leise);display:flex;align-items:center;gap:8px}
  h2::after{content:'';flex:1;height:1px;background:linear-gradient(90deg,var(--rand2),transparent)}
  h3{margin:0 0 10px;font-size:14px;font-weight:650;display:flex;align-items:center;gap:8px}
  .karte{background:linear-gradient(180deg,var(--karte2),var(--karte));border:1px solid var(--rand);border-radius:var(--r);padding:16px;box-shadow:0 10px 30px -18px #000}
  .raster{display:grid;gap:14px}
  .r2{grid-template-columns:minmax(300px,1fr) 1.35fr}
  .r3{grid-template-columns:repeat(3,1fr)}
  @media(max-width:860px){.r2,.r3{grid-template-columns:1fr}}
  /* ---- Knoepfe ---- */
  button{font:600 14px system-ui,sans-serif;color:var(--text);background:var(--karte2);border:1px solid var(--rand2);border-radius:14px;height:60px;cursor:pointer;touch-action:none;
         display:flex;flex-direction:column;align-items:center;justify-content:center;gap:2px;transition:background .15s,border-color .15s,box-shadow .2s,transform .08s}
  button:hover{border-color:#46505e;background:#222833}
  button:active{transform:scale(.97)}
  button small{color:var(--leise);font-weight:400;font-size:11px}
  button.an{border-color:var(--akzent);background:#1b2a44;box-shadow:0 0 0 1px var(--akzent) inset,0 0 18px -4px var(--akzent)}
  button.gelb.an{border-color:var(--gelb);background:#33300f;box-shadow:0 0 0 1px var(--gelb) inset,0 0 18px -4px var(--gelb)}
  button.orange.an{border-color:var(--warn);background:#3a2810;box-shadow:0 0 0 1px var(--warn) inset,0 0 18px -4px var(--warn)}
  button.rot.an{border-color:var(--rot);background:#3a1818;box-shadow:0 0 0 1px var(--rot) inset,0 0 18px -4px var(--rot)}
  .klein{height:42px;flex-direction:row;gap:8px;padding:0 14px}
  .gruppe{margin-bottom:14px}
  .gruppe:last-child{margin-bottom:0}
  .gruppe>.leise{display:block;margin:0 0 6px 2px;font-size:12px}
  .knoepfe{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
  .chips{display:flex;gap:8px;flex-wrap:wrap}
  .chips button{height:40px;width:auto;padding:0 14px;flex-direction:row;gap:8px;border-radius:999px;font-size:13px}
  .chips button i{width:12px;height:12px;border-radius:50%;display:inline-block}
  #m-1.an{animation:blau .5s infinite alternate}
  @keyframes blau{from{border-color:#3d6bff;box-shadow:0 0 18px -4px #3d6bff}to{border-color:var(--rot);box-shadow:0 0 18px -4px var(--rot)}}
  #r-rec.an{border-color:var(--rot);animation:rec 1s infinite}
  @keyframes rec{50%{border-color:transparent}}
  #r-play.an{border-color:var(--gut);box-shadow:0 0 18px -4px var(--gut)}
  .reihe{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
  .reihe button{flex:1;min-width:84px;height:48px;flex-direction:row;gap:6px;padding:0 8px}
  @media(max-width:600px){
    .kopf{position:static;gap:8px}
    .pillen{order:3;width:100%;flex-wrap:nowrap;overflow-x:auto;scrollbar-width:none}
    .pillen::-webkit-scrollbar{display:none}
    .pille{padding:5px 10px;font-size:12px}
    .knoepfe button{font-size:13px}
    main{gap:20px;padding-top:12px}
    #b-einst span{display:none}
    .marke{font-size:17px}
  }
  /* ---- Joystick ---- */
  .fahrkarte{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px}
  #feld{width:260px;height:260px;border-radius:50%;position:relative;touch-action:none;border:1px solid var(--rand2);
        background:radial-gradient(circle,#1e2430 0%,#151920 62%,#11151b 100%);box-shadow:inset 0 0 40px #000a,0 0 0 8px #ffffff05}
  #feld::before,#feld::after{content:'';position:absolute;background:#ffffff0d}
  #feld::before{left:50%;top:18px;bottom:18px;width:1px}
  #feld::after{top:50%;left:18px;right:18px;height:1px}
  #knopf{width:84px;height:84px;border-radius:50%;position:absolute;left:88px;top:88px;z-index:1;
         background:radial-gradient(circle at 35% 30%,#8ec0ff,var(--akzent) 45%,var(--akzent2));box-shadow:0 8px 24px -4px #4f9cffaa,inset 0 -4px 10px #0004}
  kbd{font:600 11px ui-monospace,monospace;background:#232934;border:1px solid var(--rand2);border-bottom-width:2px;border-radius:6px;padding:1px 6px;color:var(--text)}
  /* ---- Live ---- */
  .live{display:grid;grid-template-columns:minmax(260px,1fr) 1fr 1fr;gap:14px}
  .live .gross{grid-row:span 2;display:flex;flex-direction:column;align-items:center;gap:8px}
  .live .wert{display:flex;flex-direction:column;align-items:center;gap:4px;min-width:0}
  .live .wert b{font-size:19px}
  .live .breit{grid-column:1/-1}
  @media(max-width:860px){.live{grid-template-columns:1fr 1fr}.live .gross{grid-column:1/-1;grid-row:auto}}
  .live canvas{width:100%;max-width:140px;height:auto}
  .breit canvas{max-width:none;width:100%;height:110px}
  .neig{width:100%;max-width:140px;height:auto;overflow:visible}
  .neig .boden{stroke:#2c3139;stroke-width:1.5}
  #liveauto{width:100%;max-width:250px;height:auto}
  #liveauto .led{transition:fill .15s}
  .radglow{fill:none;stroke-width:3;opacity:0;transition:opacity .15s,stroke .15s}
  .pfeil{opacity:0;transition:opacity .25s;transform-box:fill-box;transform-origin:center}
  .pfeil.an{opacity:1;animation:pfeilLauf .7s ease-in-out infinite alternate}
  .pfeil.dreh{opacity:1;animation:pfeilDreh 1.2s ease-in-out}
  @keyframes pfeilLauf{from{transform:translateY(8px)}to{transform:translateY(-8px)}}
  @keyframes pfeilDreh{0%{transform:rotate(180deg)}60%{transform:rotate(0)}100%{transform:rotate(0);opacity:0}}
  #mt-box{width:100%;background:#13213a;border:1px solid var(--akzent);border-radius:14px;padding:12px 14px;display:flex;flex-direction:column;gap:10px;box-shadow:0 0 24px -8px var(--akzent)}
  #mt-box[hidden]{display:none}
  #mt-text{font-size:14px;line-height:1.45}
  #mt-antw{display:grid;grid-template-columns:1fr 1fr;gap:8px}
  #mt-antw button{height:auto;min-height:46px;padding:6px 8px;font-size:13px}
  #mt-antw button.gut{border-color:var(--gut)}
  /* ---- Einstellungen ---- */
  details.einst>summary{list-style:none;cursor:pointer}
  details.einst>summary::-webkit-details-marker{display:none}
  details.einst{scroll-margin-top:70px}
  .einstleiste{display:flex;align-items:center;gap:14px;padding:16px 18px;border-radius:var(--r);cursor:pointer;
    background:linear-gradient(120deg,#1b2a48,#22204a);border:1px solid #3a4f80;box-shadow:0 10px 30px -14px #4f9cff88;transition:border-color .2s,box-shadow .2s}
  .einstleiste:hover{border-color:var(--akzent);box-shadow:0 10px 34px -10px #4f9cffaa}
  .einstleiste .zahnrad{font-size:26px;width:48px;height:48px;border-radius:14px;display:grid;place-items:center;flex:none;
    background:linear-gradient(135deg,var(--akzent),var(--akzent2));box-shadow:0 6px 16px -6px #4f9cff}
  details[open] .einstleiste .zahnrad{animation:drehen .6s ease}
  @keyframes drehen{from{transform:rotate(-90deg)}to{transform:rotate(0)}}
  .einstleiste b{display:block;font-size:17px}
  .einstleiste .pfeilauf{margin-left:auto;font-size:20px;color:var(--akzent);transition:transform .25s}
  details[open] .einstleiste .pfeilauf{transform:rotate(180deg)}
  details[open] .einstleiste{margin-bottom:14px}
  #b-einst{height:34px;width:auto;padding:0 12px;border-radius:999px;flex-direction:row;gap:6px;font-size:13px}
  .einstraster{display:grid;grid-template-columns:1fr 1fr;gap:14px}
  @media(max-width:860px){.einstraster{grid-template-columns:1fr}}
  .einstraster .karte{display:flex;flex-direction:column;gap:14px}
  .regel{display:flex;flex-direction:column;gap:6px;font-size:14px}
  .regel span{display:flex;justify-content:space-between;color:var(--leise)}
  .regel span b{color:var(--text);font-weight:600}
  input[type=range]{width:100%;accent-color:var(--akzent);height:22px}
  .schalter{display:flex;align-items:center;justify-content:space-between;gap:12px;font-size:14px;color:var(--text);cursor:pointer}
  .schalter input{appearance:none;-webkit-appearance:none;flex:none;width:44px;height:26px;border-radius:13px;background:#323a45;position:relative;cursor:pointer;transition:background .2s;margin:0}
  .schalter input::before{content:'';position:absolute;top:3px;left:3px;width:20px;height:20px;border-radius:50%;background:#fff;transition:transform .2s;box-shadow:0 1px 3px #0006}
  .schalter input:checked{background:linear-gradient(90deg,var(--akzent),var(--akzent2))}
  .schalter input:checked::before{transform:translateX(18px)}
  .wahl{display:flex;gap:6px}
  .wahl button{height:38px;flex:1;font-size:13px;border-radius:10px}
  .hinweis{color:var(--leise);font-size:12.5px;margin-top:-6px}
  #e-status{color:var(--gut);font-size:13px;min-height:18px}
  .updinfo{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
  @media(max-width:600px){.updinfo{grid-template-columns:1fr 1fr}}
  .updinfo div{background:#12161c;border:1px solid var(--rand);border-radius:12px;padding:10px 12px;display:flex;flex-direction:column;gap:2px}
  .updinfo b{font-size:15px}
  button.upd-neu{border-color:var(--gut);background:#12301f;box-shadow:0 0 18px -6px var(--gut)}
  .pille.upd{border-color:var(--gut);color:#7fe0a3;cursor:pointer}
  /* ---- Jukebox ---- */
  .jukebox{display:grid;grid-template-columns:minmax(260px,340px) 1fr;gap:14px}
  @media(max-width:860px){.jukebox{grid-template-columns:1fr}}
  .player{display:flex;flex-direction:column;gap:12px;background:radial-gradient(400px 200px at 50% 0%,#2a1f55,transparent 70%),linear-gradient(180deg,var(--karte2),var(--karte))}
  .player .titel{font-size:17px;font-weight:700;min-height:22px}
  #viz{width:100%;height:110px;display:block}
  .laufbalken{height:6px;border-radius:3px;background:#2a3039;overflow:hidden}
  #lied-balken{height:100%;width:0;background:linear-gradient(90deg,#ff4fd8,var(--akzent2),var(--akzent));transition:width .25s linear}
  .lieder{display:grid;grid-template-columns:repeat(auto-fill,minmax(210px,1fr));gap:10px}
  .lied{height:auto;min-height:64px;flex-direction:row;justify-content:flex-start;gap:12px;padding:10px 12px;text-align:left}
  .lied .ico{font-size:24px;width:42px;height:42px;border-radius:12px;display:grid;place-items:center;background:#ffffff0a;flex:none}
  .lied span{display:flex;flex-direction:column;gap:1px;min-width:0}
  .lied small{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .lied.an{border-color:#ff4fd8;background:#2c1838;box-shadow:0 0 0 1px #ff4fd8 inset,0 0 22px -6px #ff4fd8}
  .lied.an .ico{animation:hops .45s ease-in-out infinite alternate}
  @keyframes hops{to{transform:translateY(-3px) scale(1.08)}}
  .nav{display:flex;gap:4px;width:100%;justify-content:center;flex-wrap:wrap}
  .nav a{color:var(--leise);font-size:13px;padding:4px 10px;border-radius:999px}
  .nav a:hover{color:var(--text);background:#ffffff0d}
  @media(max-width:600px){.nav{display:none}}
  .fuss{text-align:center;color:var(--leise);font-size:12.5px;display:flex;flex-direction:column;gap:6px;align-items:center}
</style></head><body>
<header class="kopf">
  <div class="marke"><svg viewBox="-66 -18 132 60" width="52" height="24" aria-hidden="true"><rect x="-40" y="-14" width="80" height="4" fill="#e6e6e6" stroke="#222" stroke-width=".6"/><rect x="-40" y="-10" width="80" height="12" fill="#ffd54f" stroke="#222" stroke-width=".6"/><rect x="-40" y="2" width="80" height="17" fill="#404040" stroke="#222" stroke-width=".6"/><circle cx="-33" cy="10" r="27" fill="#404040" stroke="#111" stroke-width=".8"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(0 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(20 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(40 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(60 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(80 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(100 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(120 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(140 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(160 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(180 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(200 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(220 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(240 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(260 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(280 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(300 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(320 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(340 -33 10)"/><circle cx="-33" cy="10" r="17" fill="#383838" stroke="#1e1e1e" stroke-width="1.2"/><circle cx="-33" cy="10" r="14" fill="none" stroke="#2a2a2a" stroke-width="1"/><line x1="-29.5" y1="12.0" x2="-20.9" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="-33.0" y1="14.0" x2="-33.0" y2="24.0" stroke="#222" stroke-width="1.6"/><line x1="-36.5" y1="12.0" x2="-45.1" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="-36.5" y1="8.0" x2="-45.1" y2="3.0" stroke="#222" stroke-width="1.6"/><line x1="-33.0" y1="6.0" x2="-33.0" y2="-4.0" stroke="#222" stroke-width="1.6"/><line x1="-29.5" y1="8.0" x2="-20.9" y2="3.0" stroke="#222" stroke-width="1.6"/><circle cx="-33" cy="10" r="4.5" fill="#c8cde0" stroke="#222" stroke-width=".8"/><circle cx="33" cy="10" r="27" fill="#404040" stroke="#111" stroke-width=".8"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(0 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(20 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(40 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(60 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(80 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(100 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(120 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(140 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(160 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(180 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(200 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(220 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(240 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(260 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(280 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(300 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(320 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(340 33 10)"/><circle cx="33" cy="10" r="17" fill="#383838" stroke="#1e1e1e" stroke-width="1.2"/><circle cx="33" cy="10" r="14" fill="none" stroke="#2a2a2a" stroke-width="1"/><line x1="36.5" y1="12.0" x2="45.1" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="33.0" y1="14.0" x2="33.0" y2="24.0" stroke="#222" stroke-width="1.6"/><line x1="29.5" y1="12.0" x2="20.9" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="29.5" y1="8.0" x2="20.9" y2="3.0" stroke="#222" stroke-width="1.6"/><line x1="33.0" y1="6.0" x2="33.0" y2="-4.0" stroke="#222" stroke-width="1.6"/><line x1="36.5" y1="8.0" x2="45.1" y2="3.0" stroke="#222" stroke-width="1.6"/><circle cx="33" cy="10" r="4.5" fill="#c8cde0" stroke="#222" stroke-width=".8"/></svg>Roboauto</div>
  <div class="pillen">
    <div class="pille"><span id="punkt"></span><span id="verb">verbinde...</span></div>
    <div class="pille"><span class="t">Akku</span><b id="akku">-</b><div class="akkuleiste"><div id="akkubalken"></div></div></div>
    <div class="pille"><span class="t">WLAN</span><b id="wlan">-</b></div>
    <div class="pille"><span class="t">Motoren L / R</span><b id="mot">-</b></div>
    <div class="pille upd" id="upd-pille" hidden>&#11014;&#65039; <span>Update</span></div>
  </div>
  <button id="b-einst" title="Einstellungen">&#9881;&#65039; <span>Einstellungen</span></button>
  <button id="b-sprache" title="Sprache / Language">&#127760; <span id="sprache-name">English</span></button>
  <nav class="nav"><a href="#steuerung">Steuerung</a><a href="#live">Live</a><a href="#programme">Programme</a><a href="#musik">&#127925; Musik</a><a href="#einst">Einstellungen</a></nav>
</header>

<main>
<section id="steuerung">
  <h2>Steuerung</h2>
  <div class="raster r2">
    <div class="karte fahrkarte">
      <div id="feld"><div id="knopf"></div></div>
      <div class="leise"><kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> / <span>Pfeiltasten</span> &middot; <span>Loslassen = Stopp</span></div>
    </div>
    <div class="karte">
      <div class="gruppe"><span class="leise">Beleuchtung</span>
        <div class="knoepfe">
          <button id="b-licht" class="gelb"   data-aktion="1">Licht<small>L</small></button>
          <button id="b-fern"  class="gelb"   data-aktion="2">Fernlicht<small>F</small></button>
          <button id="b-warn"  class="orange" data-aktion="3">Warnblinker<small>B</small></button>
        </div>
      </div>
      <div class="gruppe"><span class="leise">Signale</span>
        <div class="knoepfe">
          <button id="b-bl" class="orange" data-halt="1">&#9664; Blinker<small>Q halten</small></button>
          <button id="b-hupe"               data-halt="4">Hupe<small>H / Leertaste</small></button>
          <button id="b-br" class="orange" data-halt="2">Blinker &#9654;<small>E halten</small></button>
        </div>
      </div>
      <div class="gruppe"><span class="leise">Man&ouml;ver &amp; Anzeige</span>
        <div class="knoepfe">
          <button id="b-wende" class="rot" data-aktion="4">180&deg; Wende<small>U</small></button>
          <button id="b-tank"              data-halt="8">Tank<small>T halten</small></button>
          <button id="b-kal"               data-aktion="33">&#127919; Kalibrieren<small>Gyro</small></button>
        </div>
      </div>
      <div class="gruppe"><span class="leise">Modus &middot; <kbd>P</kbd> = n&auml;chster</span>
        <div class="chips">
          <button id="m-0" data-aktion="10"><i style="background:conic-gradient(red,yellow,lime,cyan,blue,magenta,red)"></i>Normal</button>
          <button id="m-1" data-aktion="11"><i style="background:linear-gradient(90deg,#3d6bff 50%,#ef5350 50%)"></i>Polizei</button>
          <button id="m-2" data-aktion="12"><i style="background:#2f6bff"></i>Team Blau</button>
          <button id="m-3" data-aktion="13"><i style="background:#2ecc55"></i>Team Gr&uuml;n</button>
          <button id="m-4" data-aktion="14"><i style="background:#f2c200"></i>Team Gelb</button>
          <button id="m-5" data-aktion="15"><i style="background:#e53935"></i>Team Rot</button>
        </div>
      </div>
    </div>
  </div>
</section>

<section id="live">
  <h2>Live</h2>
  <div class="live">
    <div class="karte gross">
      <span class="leise">Live-Ansicht von oben &middot; Lichter und Ring leuchten wie am Auto</span>
      <svg id="liveauto" viewBox="0 0 220 228"><defs><pattern id="profilL" width="48" height="9" patternUnits="userSpaceOnUse"><path d="M-4 5 q13 -6 26 0 t26 0" stroke="#4a4a4a" stroke-width="2.4" fill="none"/></pattern><pattern id="profilR" width="48" height="9" patternUnits="userSpaceOnUse"><path d="M-4 5 q13 -6 26 0 t26 0" stroke="#4a4a4a" stroke-width="2.4" fill="none"/></pattern></defs><text x="110" y="9" text-anchor="middle" font-size="9" fill="#9aa0a6">vorne</text><rect x="4" y="18" width="46" height="92" rx="14" fill="#151515" stroke="#000" stroke-width="1"/><rect x="4" y="18" width="46" height="92" rx="14" fill="url(#profilL)"/><rect class="radglow gl" x="4" y="18" width="46" height="92" rx="14"/><rect x="170" y="18" width="46" height="92" rx="14" fill="#151515" stroke="#000" stroke-width="1"/><rect x="170" y="18" width="46" height="92" rx="14" fill="url(#profilR)"/><rect class="radglow gr" x="170" y="18" width="46" height="92" rx="14"/><rect x="4" y="120" width="46" height="92" rx="14" fill="#151515" stroke="#000" stroke-width="1"/><rect x="4" y="120" width="46" height="92" rx="14" fill="url(#profilL)"/><rect class="radglow gl" x="4" y="120" width="46" height="92" rx="14"/><rect x="170" y="120" width="46" height="92" rx="14" fill="#151515" stroke="#000" stroke-width="1"/><rect x="170" y="120" width="46" height="92" rx="14" fill="url(#profilR)"/><rect class="radglow gr" x="170" y="120" width="46" height="92" rx="14"/><rect x="48" y="58" width="4" height="7" fill="#c8cde0"/><rect x="168" y="58" width="4" height="7" fill="#c8cde0"/><rect x="48" y="166" width="4" height="7" fill="#c8cde0"/><rect x="168" y="166" width="4" height="7" fill="#c8cde0"/><path d="M48 56 Q48 44 60 44 H160 Q172 44 172 56 V190 H122 V186 H98 V190 H48 Z" fill="#e6e6e6" stroke="#222" stroke-width="1.2"/><rect x="99" y="185" width="22" height="4" rx="1.5" fill="#f2c200" stroke="#222" stroke-width=".6"/><rect class="led" id="lv0" x="64" y="38" width="18" height="5" rx="2"/><rect class="led" id="lv1" x="138" y="38" width="18" height="5" rx="2"/><rect class="led" id="lv2" x="64" y="191" width="18" height="5" rx="2"/><rect class="led" id="lv3" x="138" y="191" width="18" height="5" rx="2"/><circle cx="110" cy="117" r="31" fill="#d3d5d8" stroke="#b5b8bc" stroke-width="1.5"/><circle cx="110" cy="117" r="22" fill="#e6e6e6"/><g id="ringleds"></g><g class="pfeil" id="pfeilL"><path d="M27 78 l15 18 h-8 v38 h-14 v-38 h-8 z" fill="#3ccf6e" stroke="#0b2a16" stroke-width="1.5"/></g><g class="pfeil" id="pfeilR"><path d="M193 78 l15 18 h-8 v38 h-14 v-38 h-8 z" fill="#3ccf6e" stroke="#0b2a16" stroke-width="1.5"/></g></svg>
      <div id="mt-box" hidden>
        <b id="mt-titel">&#128295; Motortest-Assistent</b>
        <div id="mt-text"></div>
        <div id="mt-antw"></div>
      </div>
    </div>
    <div class="karte wert"><span class="leise">Richtung</span><canvas id="c-kompass" width="150" height="150"></canvas><b id="t-kurs">-</b></div>
    <div class="karte wert"><span class="leise">Motorleistung</span><canvas id="c-tempo" width="150" height="150"></canvas><b id="t-tempo">-</b></div>
    <div class="karte wert"><span class="leise">Neigung vorn/hinten</span><svg class="neig" viewBox="-75 -75 150 150"><line class="boden" x1="-70" y1="38" x2="70" y2="38"/><g id="g-nicken"><rect x="-40" y="-14" width="80" height="4" fill="#e6e6e6" stroke="#222" stroke-width=".6"/><rect x="-40" y="-10" width="80" height="12" fill="#ffd54f" stroke="#222" stroke-width=".6"/><rect x="-40" y="2" width="80" height="17" fill="#404040" stroke="#222" stroke-width=".6"/><circle cx="-33" cy="10" r="27" fill="#404040" stroke="#111" stroke-width=".8"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(0 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(20 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(40 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(60 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(80 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(100 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(120 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(140 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(160 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(180 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(200 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(220 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(240 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(260 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(280 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(300 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(320 -33 10)"/><rect x="-36.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(340 -33 10)"/><circle cx="-33" cy="10" r="17" fill="#383838" stroke="#1e1e1e" stroke-width="1.2"/><circle cx="-33" cy="10" r="14" fill="none" stroke="#2a2a2a" stroke-width="1"/><line x1="-29.5" y1="12.0" x2="-20.9" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="-33.0" y1="14.0" x2="-33.0" y2="24.0" stroke="#222" stroke-width="1.6"/><line x1="-36.5" y1="12.0" x2="-45.1" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="-36.5" y1="8.0" x2="-45.1" y2="3.0" stroke="#222" stroke-width="1.6"/><line x1="-33.0" y1="6.0" x2="-33.0" y2="-4.0" stroke="#222" stroke-width="1.6"/><line x1="-29.5" y1="8.0" x2="-20.9" y2="3.0" stroke="#222" stroke-width="1.6"/><circle cx="-33" cy="10" r="4.5" fill="#c8cde0" stroke="#222" stroke-width=".8"/><circle cx="33" cy="10" r="27" fill="#404040" stroke="#111" stroke-width=".8"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(0 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(20 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(40 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(60 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(80 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(100 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(120 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(140 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(160 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(180 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(200 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(220 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(240 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(260 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(280 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(300 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(320 33 10)"/><rect x="29.5" y="-20" width="7" height="4.5" rx="1.5" fill="#111" transform="rotate(340 33 10)"/><circle cx="33" cy="10" r="17" fill="#383838" stroke="#1e1e1e" stroke-width="1.2"/><circle cx="33" cy="10" r="14" fill="none" stroke="#2a2a2a" stroke-width="1"/><line x1="36.5" y1="12.0" x2="45.1" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="33.0" y1="14.0" x2="33.0" y2="24.0" stroke="#222" stroke-width="1.6"/><line x1="29.5" y1="12.0" x2="20.9" y2="17.0" stroke="#222" stroke-width="1.6"/><line x1="29.5" y1="8.0" x2="20.9" y2="3.0" stroke="#222" stroke-width="1.6"/><line x1="33.0" y1="6.0" x2="33.0" y2="-4.0" stroke="#222" stroke-width="1.6"/><line x1="36.5" y1="8.0" x2="45.1" y2="3.0" stroke="#222" stroke-width="1.6"/><circle cx="33" cy="10" r="4.5" fill="#c8cde0" stroke="#222" stroke-width=".8"/></g></svg><b id="t-nicken">-</b></div>
    <div class="karte wert"><span class="leise">Neigung seitlich</span><svg class="neig" viewBox="-75 -75 150 150"><line class="boden" x1="-70" y1="38" x2="70" y2="38"/><g id="g-rollen"><defs><pattern id="profilH" width="48" height="9" patternUnits="userSpaceOnUse"><path d="M-4 5 q13 -6 26 0 t26 0" stroke="#4a4a4a" stroke-width="2.4" fill="none"/></pattern></defs><rect x="-36" y="-17" width="72" height="5" fill="#e6e6e6" stroke="#222" stroke-width=".6"/><rect x="-34" y="-12" width="68" height="16" fill="#ffd54f" stroke="#222" stroke-width=".6"/><rect x="-34" y="4" width="68" height="15" fill="#404040" stroke="#222" stroke-width=".6"/><rect x="-24" y="-7" width="10" height="6" rx="1" fill="#8a6d1e" stroke="#222" stroke-width=".6"/><rect x="14" y="-7" width="10" height="6" rx="1" fill="#8a6d1e" stroke="#222" stroke-width=".6"/><rect class="led" id="rv2" x="-22" y="-5.5" width="6" height="3" rx="1"/><rect class="led" id="rv3" x="16" y="-5.5" width="6" height="3" rx="1"/><rect x="-7" y="-18" width="14" height="6" fill="#d9d9d9" stroke="#222" stroke-width=".6"/><rect x="-6" y="-11" width="12" height="4.5" rx="2.2" fill="#111" stroke="#666" stroke-width=".6"/><rect x="-38" y="9" width="4" height="4" fill="#c8cde0"/><rect x="34" y="9" width="4" height="4" fill="#c8cde0"/><rect x="-61" y="-22" width="24" height="62" rx="10" fill="#151515" stroke="#000" stroke-width=".8"/><rect x="-61" y="-22" width="24" height="62" rx="10" fill="url(#profilH)"/><rect x="37" y="-22" width="24" height="62" rx="10" fill="#151515" stroke="#000" stroke-width=".8"/><rect x="37" y="-22" width="24" height="62" rx="10" fill="url(#profilH)"/></g></svg><b id="t-rollen">-</b></div>
    <div class="karte wert breit"><span class="leise">Akkuverlauf (letzte Stunde)</span><canvas id="c-akku" width="1040" height="110"></canvas></div>
  </div>
</section>

<section id="programme">
  <h2>Programme</h2>
  <div class="raster r3">
    <div class="karte"><h3>&#10024; Figuren</h3>
      <div class="reihe">
        <button id="f-0" data-aktion="20">&#9634; Quadrat</button>
        <button id="f-1" data-aktion="21">&infin; Acht</button>
        <button id="f-2" data-aktion="22">&#8767; Slalom</button>
      </div>
      <p class="hinweis" style="margin:10px 0 0">Nochmal klicken oder lenken = Stopp.</p>
    </div>
    <div class="karte"><h3>&#127916; Fahrweg</h3>
      <div class="reihe">
        <button id="r-rec"  data-aktion="30">&#9679; Aufnahme</button>
        <button id="r-play" data-aktion="31">&#9654; Abspielen</button>
      </div>
      <p class="hinweis" id="r-info" style="margin:10px 0 0"></p>
    </div>
    <div class="karte"><h3>&#129514; Experiment</h3>
      <div class="reihe"><button id="b-balance" data-aktion="32">&#9878; Balancieren</button></div>
      <p class="hinweis" style="margin:10px 0 0">Auto am Balancierpunkt halten, dann klicken. Nochmal klicken = aus.</p>
    </div>
  </div>
</section>

<section id="musik">
  <h2>Jukebox</h2>
  <div class="jukebox">
    <div class="karte player">
      <span class="leise">Jetzt l&auml;uft</span>
      <div class="titel" id="lied-titel">&ndash;</div>
      <canvas id="viz" width="600" height="220"></canvas>
      <div class="laufbalken"><div id="lied-balken"></div></div>
      <button id="lied-stopp" class="klein">&#9632; Stopp</button>
      <span class="hinweis" style="margin:0">Spielt &uuml;ber den Buzzer im Auto, der LED-Ring macht die Lichtshow dazu. Nochmal auf das Lied klicken = Stopp.</span>
    </div>
    <div class="lieder" id="lieder"></div>
  </div>
</section>

<details class="einst" id="einst">
  <summary class="einstleiste"><span class="zahnrad">&#9881;&#65039;</span><span><b>Einstellungen</b><span class="leise">Tempo, Lenkung, Sicherheit, Licht &amp; Ton, Motoren, WLAN</span></span><span class="pfeilauf">&#9662;</span></summary>
  <div class="einstraster">
    <div class="karte"><h3>&#127950;&#65039; Fahren</h3>
      <label class="regel"><span>H&ouml;chstgeschwindigkeit <b id="v-tempo"></b></span><input type="range" id="e-tempo" min="40" max="255"></label>
      <label class="regel"><span>Lenkung beim Fahren <b id="v-lenkung"></b></span><input type="range" id="e-lenkung" min="20" max="255"></label>
      <label class="regel"><span>Drehen auf der Stelle <b id="v-lenkungStand"></b></span><input type="range" id="e-lenkungStand" min="40" max="255"></label>
      <label class="regel"><span>Figurengr&ouml;&szlig;e <b id="v-figurGroesse"></b></span><input type="range" id="e-figurGroesse" min="50" max="200" step="10"></label>
      <label class="schalter">Gyro-Geradeauslauf <input type="checkbox" id="e-geradeaus"></label>
    </div>
    <div class="karte"><h3>&#128737;&#65039; Sicherheit</h3>
      <label class="schalter">Crash- und Umkipp-Erkennung <input type="checkbox" id="e-crash"></label>
      <label class="regel"><span>Crash-Empfindlichkeit <b id="v-crashG"></b></span><input type="range" id="e-crashG" min="1" max="6" step="0.1"></label>
      <p class="hinweis" style="margin:0">Gyro kalibrieren: Auto dazu auf den Boden stellen und 1 Sekunde nicht ber&uuml;hren. Setzt auch den Kompass auf 0&deg;.</p>
    </div>
    <div class="karte"><h3>&#128161; Licht &amp; Ton</h3>
      <label class="regel"><span>Helligkeit LED-Ring <b id="v-ringHell"></b></span><input type="range" id="e-ringHell" min="5" max="255"></label>
      <label class="regel"><span>Helligkeit Lichter <b id="v-lichtHell"></b></span><input type="range" id="e-lichtHell" min="5" max="255"></label>
      <label class="regel"><span>&#128266; Lautst&auml;rke <b id="v-lautstaerke"></b></span><input type="range" id="e-lautstaerke" min="5" max="100" step="5"></label>
      <div class="regel"><span>T&ouml;ne</span>
        <div class="wahl"><button data-toene="2">Alle</button><button data-toene="1">Nur wichtige</button><button data-toene="0">Stumm</button></div>
      </div>
    </div>
    <div class="karte"><h3>&#9881;&#65039; Motoren</h3>
      <label class="schalter">Linke Seite umkehren <input type="checkbox" id="e-motorL"></label>
      <label class="schalter">Rechte Seite umkehren <input type="checkbox" id="e-motorR"></label>
      <label class="schalter">Links und rechts tauschen <input type="checkbox" id="e-motorTausch"></label>
      <button id="b-mtest" class="klein">&#128295; Motortest-Assistent starten</button>
      <p class="hinweis" style="margin:0">Der Assistent dreht nacheinander kurz jede Seite und fragt, was passiert ist. Die richtigen Schalter stellt er dann selbst ein. Auto dazu am besten hochheben.</p>
    </div>
    <div class="karte" style="grid-column:1/-1">
      <h3 style="margin:0">&#128260; Updates</h3>
      <div class="updinfo">
        <div><span class="leise">Installiert</span><b id="fw-ist">&ndash;</b></div>
        <div><span class="leise">Auf GitHub</span><b id="fw-neu">&ndash;</b></div>
        <div><span class="leise">Status</span><b id="upd-status">&ndash;</b></div>
      </div>
      <label class="schalter">Automatisch aktualisieren <input type="checkbox" id="e-autoUpdate"></label>
      <div class="reihe">
        <button id="upd-suchen" class="klein">&#128269; Nach Updates suchen</button>
        <button id="upd-install" class="klein upd-neu" hidden>&#11015;&#65039; Jetzt installieren</button>
      </div>
      <p class="hinweis" id="upd-text" style="margin:0">Neue Versionen kommen automatisch von GitHub. Installiert wird nur, wenn das Auto 2 Minuten unbenutzt steht und der Akku reicht. Startet eine neue Version nicht sauber, schaltet das Auto von selbst auf die alte zur&uuml;ck.</p>
    </div>
    <div class="karte" style="grid-column:1/-1;flex-direction:row;flex-wrap:wrap;align-items:center;justify-content:space-between">
      <h3 style="margin:0">&#128421;&#65039; System</h3>
      <span id="e-status"></span>
      <div class="reihe" style="flex:0 1 auto">
        <button class="klein" onclick="location.href='/wlan'">&#128246; WLAN &auml;ndern</button>
        <button id="e-standard" class="klein">&#8634; Standard wiederherstellen</button>
      </div>
    </div>
  </div>
</details>

<footer class="fuss">
  <div><span>Tastatur:</span> <kbd>L</kbd> <span>Licht</span> &middot; <kbd>F</kbd> <span>Fernlicht</span> &middot; <kbd>B</kbd> <span>Warnblinker</span> &middot; <kbd>U</kbd> <span>Wende</span> &middot; <kbd>Q</kbd>/<kbd>E</kbd> <span>Blinker</span> &middot; <kbd>H</kbd> <span>Hupe</span> &middot; <kbd>T</kbd> <span>Tank</span> &middot; <kbd>P</kbd> <span>Modus</span></div>
  <div id="version"><span>Software: Schritt</span> 30<span id="version-build"></span></div>
</footer>
</main>

<script>
const MAX = 7;
let jx = 0, jy = 0;            // Joystick (Maus/Touch)
const keys = {};               // gedrueckte Fahrtasten
let halt = 0;                  // gehaltene Funktionstasten (Bits)
const haltMaus = {};           // per Maus/Touch gehaltene Knoepfe
let unterwegs = false, letzteGesendet = "0,0,0";

// ---------- Joystick ----------
const feld = document.getElementById('feld'), knopf = document.getElementById('knopf');
let ziehen = false;
function setzeKnopf(px, py){ knopf.style.left = (88+px)+'px'; knopf.style.top = (88+py)+'px'; }
function bewege(e){
  const r = feld.getBoundingClientRect();
  let dx = e.clientX - (r.left + r.width/2), dy = e.clientY - (r.top + r.height/2);
  const max = 88, d = Math.hypot(dx, dy);
  if (d > max){ dx *= max/d; dy *= max/d; }
  setzeKnopf(dx, dy);
  jx = Math.round(dx/max*MAX); jy = Math.round(-dy/max*MAX);
}
feld.addEventListener('pointerdown', e => { ziehen = true; feld.setPointerCapture(e.pointerId); bewege(e); });
feld.addEventListener('pointermove', e => { if (ziehen) bewege(e); });
const loslassen = () => { ziehen = false; jx = 0; jy = 0; setzeKnopf(0, 0); };
feld.addEventListener('pointerup', loslassen); feld.addEventListener('pointercancel', loslassen);

// ---------- Knoepfe ----------
function aktion(a){ fetch('/aktion?a=' + a).catch(()=>{}); }
document.querySelectorAll('button[data-aktion]').forEach(b =>
  b.addEventListener('click', () => aktion(b.dataset.aktion)));
document.querySelectorAll('button[data-halt]').forEach(b => {
  const bit = +b.dataset.halt;
  const an  = e => { haltMaus[bit] = true;  b.setPointerCapture?.(e.pointerId); };
  const aus = () => { haltMaus[bit] = false; };
  b.addEventListener('pointerdown', an);
  b.addEventListener('pointerup', aus); b.addEventListener('pointercancel', aus); b.addEventListener('pointerleave', aus);
});

// ---------- Tastatur ----------
const fahrMap = {ArrowUp:'w',ArrowDown:'s',ArrowLeft:'a',ArrowRight:'d',w:'w',a:'a',s:'s',d:'d'};
const haltMap = {q:1, e:2, h:4, ' ':4, t:8};
const aktionMap = {l:1, f:2, b:3, u:4, p:5};
const haltTaste = {};
addEventListener('keydown', e => {
  const k = e.key.length === 1 ? e.key.toLowerCase() : e.key;
  if (fahrMap[k]) { keys[fahrMap[k]] = true; e.preventDefault(); }
  else if (haltMap[k]) { haltTaste[haltMap[k]] = true; e.preventDefault(); }
  else if (aktionMap[k] && !e.repeat) { aktion(aktionMap[k]); e.preventDefault(); }
});
addEventListener('keyup', e => {
  const k = e.key.length === 1 ? e.key.toLowerCase() : e.key;
  if (fahrMap[k]) { keys[fahrMap[k]] = false; e.preventDefault(); }
  else if (haltMap[k]) { haltTaste[haltMap[k]] = false; e.preventDefault(); }
});
addEventListener('blur', () => {
  for (const k in keys) keys[k] = false;
  for (const k in haltTaste) haltTaste[k] = false;
  for (const k in haltMaus) haltMaus[k] = false;
  loslassen();
});

// ---------- Senden: alle 80 ms, solange etwas aktiv ist; einmal 0 beim Loslassen ----------
function aktuell(){
  let x = jx, y = jy;
  if (keys.w || keys.s || keys.a || keys.d){
    y = (keys.w ? MAX : 0) - (keys.s ? MAX : 0);
    x = (keys.d ? MAX : 0) - (keys.a ? MAX : 0);
  }
  let t = 0;
  for (const bit of [1, 2, 4, 8]) if (haltTaste[bit] || haltMaus[bit]) t |= bit;
  return [x, y, t];
}
function zeigeHalt(t){
  document.getElementById('b-bl').classList.toggle('an', !!(t & 1));
  document.getElementById('b-br').classList.toggle('an', !!(t & 2));
  document.getElementById('b-hupe').classList.toggle('an', !!(t & 4));
  document.getElementById('b-tank').classList.toggle('an', !!(t & 8));
}
async function sendeSchleife(){
  const [x, y, t] = aktuell(), wert = x + ',' + y + ',' + t;
  zeigeHalt(t);
  if (!unterwegs && (wert !== '0,0,0' || letzteGesendet !== '0,0,0')){
    unterwegs = true;
    try { await fetch('/fahr?x='+x+'&y='+y+'&t='+t); letzteGesendet = wert; } catch(e){}
    unterwegs = false;
  }
  setTimeout(sendeSchleife, 80);
}
sendeSchleife();

// ---------- Live-Anzeigen (Canvas) ----------
const farbe = n => getComputedStyle(document.documentElement).getPropertyValue(n).trim();
function zeichneKompass(kurs){
  const c = document.getElementById('c-kompass'), g = c.getContext('2d'), m = 75;
  g.clearRect(0, 0, 150, 150);
  g.strokeStyle = '#2c3139'; g.lineWidth = 2; g.beginPath(); g.arc(m, m, 66, 0, 7); g.stroke();
  g.fillStyle = farbe('--leise'); g.font = '12px system-ui'; g.textAlign = 'center'; g.textBaseline = 'middle';
  [['0',0],['90',90],['180',180],['270',270]].forEach(([t, w]) => {
    const r = (w - 90) * Math.PI / 180; g.fillText(t, m + Math.cos(r) * 54, m + Math.sin(r) * 54);
  });
  const r = (kurs - 90) * Math.PI / 180;
  g.save(); g.translate(m, m); g.rotate(r + Math.PI / 2);
  g.fillStyle = farbe('--akzent'); g.beginPath(); g.moveTo(0, -44); g.lineTo(9, 6); g.lineTo(0, 0); g.lineTo(-9, 6); g.closePath(); g.fill();
  g.fillStyle = '#3a4048'; g.beginPath(); g.moveTo(0, 30); g.lineTo(7, 6); g.lineTo(0, 0); g.lineTo(-7, 6); g.closePath(); g.fill();
  g.restore();
}
function zeichneNeigung(id, grad, seitlich){
  // Auto dreht sich um die Mitte der Aufstandsflaeche; ueber 30 Grad rot einfaerben
  const g = document.getElementById(id);
  g.setAttribute('transform', 'rotate(' + (-grad) + ' 0 38)');
  g.style.filter = Math.abs(grad) > 30 ? 'drop-shadow(0 0 6px #ef5350)' : '';
}
// ---------- Live-Auto ----------
const ringLeds = document.getElementById('ringleds');
for (let i = 0; i < 8; i++) {                       // 8 Ring-LEDs im Kreis, LED 0 oben, im Uhrzeigersinn
  const w = (i / 8) * 2 * Math.PI - Math.PI / 2;
  const c = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  c.setAttribute('cx', 110 + Math.cos(w) * 26.5); c.setAttribute('cy', 117 + Math.sin(w) * 26.5);
  c.setAttribute('r', 5.5); c.setAttribute('class', 'led'); c.id = 'rl' + i;
  ringLeds.appendChild(c);
}
function leuchte(el, hex){
  // Dunkle Farben (z.B. gedimmtes Ruecklicht) fuer die Anzeige etwas aufhellen
  let r = parseInt(hex.substr(0, 2), 16), g = parseInt(hex.substr(2, 2), 16), b = parseInt(hex.substr(4, 2), 16);
  const m = Math.max(r, g, b);
  if (m < 6) { el.setAttribute('fill', '#3a3d42'); el.style.filter = ''; return; }
  const k = Math.min(255 / m, 3);
  r = Math.round(r * k); g = Math.round(g * k); b = Math.round(b * k);
  const farbe = 'rgb(' + r + ',' + g + ',' + b + ')';
  el.setAttribute('fill', farbe);
  el.style.filter = 'drop-shadow(0 0 ' + (3 + 5 * m / 255) + 'px ' + farbe + ')';
}
function zeigeLiveAuto(s){
  if (!s.lampen || !s.ring) return;
  s.lampen.split(',').forEach((h, i) => {
    leuchte(document.getElementById('lv' + i), h);
    const hinten = document.getElementById('rv' + i);       // Heckansicht bei 'Neigung seitlich'
    if (hinten) leuchte(hinten, h);
  });
  s.ring.split(',').forEach((h, i) => { const el = document.getElementById('rl' + i); if (el) leuchte(el, h); });
}
function zeichneTempo(prozent){
  const c = document.getElementById('c-tempo'), g = c.getContext('2d'), m = 75;
  g.clearRect(0, 0, 150, 150);
  g.lineCap = 'round'; g.lineWidth = 12;
  g.strokeStyle = '#2c3139'; g.beginPath(); g.arc(m, m + 10, 56, Math.PI * 0.75, Math.PI * 2.25); g.stroke();
  const anteil = Math.min(Math.abs(prozent), 100) / 100;
  g.strokeStyle = prozent < 0 ? farbe('--warn') : farbe('--gut');
  if (anteil > 0){ g.beginPath(); g.arc(m, m + 10, 56, Math.PI * 0.75, Math.PI * (0.75 + 1.5 * anteil)); g.stroke(); }
  g.fillStyle = farbe('--text'); g.font = '600 22px system-ui'; g.textAlign = 'center';
  g.fillText(Math.abs(prozent) + '%', m, m + 16);
  g.fillStyle = farbe('--leise'); g.font = '12px system-ui';
  g.fillText(t(prozent < 0 ? 'rückwärts' : prozent > 0 ? 'vorwärts' : 'steht'), m, m + 36);
}
async function zeichneAkku(einmal){
  try {
    const d = await (await fetch('/verlauf')).json(), w = d.werte;
    const c = document.getElementById('c-akku'), g = c.getContext('2d'), B = c.width, H = c.height;
    g.clearRect(0, 0, B, H);
    g.strokeStyle = '#2c3139'; g.lineWidth = 1; g.fillStyle = farbe('--leise'); g.font = '11px system-ui';
    for (const p of [0, 50, 100]){ const yy = H - 14 - p / 100 * (H - 24); g.beginPath(); g.moveTo(34, yy); g.lineTo(B, yy); g.stroke(); g.fillText(p + '%', 2, yy + 4); }
    if (w.length < 2){ g.fillText(t('sammelt Daten... (alle {0} s ein Punkt)', d.intervall), 40, H / 2); return; }
    const punkte = 360, dx = (B - 36) / (punkte - 1), start = B - (w.length - 1) * dx;
    const grad = g.createLinearGradient(0, 0, 0, H);
    grad.addColorStop(0, 'rgba(60,207,110,.35)'); grad.addColorStop(1, 'rgba(60,207,110,0)');
    g.beginPath();
    w.forEach((v, i) => { const p = v === 255 ? 100 : v, xx = start + i * dx, yy = H - 14 - p / 100 * (H - 24); i ? g.lineTo(xx, yy) : g.moveTo(xx, yy); });
    g.strokeStyle = farbe('--gut'); g.lineWidth = 2; g.stroke();
    g.lineTo(start + (w.length - 1) * dx, H - 14); g.lineTo(start, H - 14); g.closePath(); g.fillStyle = grad; g.fill();
    g.fillStyle = farbe('--leise'); g.fillText(t('jetzt'), B - 28, H - 2); g.fillText(t('vor {0} min', Math.round(w.length * d.intervall / 60)), start, H - 2);
  } catch(e){}
  if (einmal !== true) setTimeout(zeichneAkku, 10000);
}
zeichneAkku();

// ---------- Einstellungen ----------
let einst = {};
const anzeige = {
  tempo:        v => Math.round(v / 255 * 100) + ' %',
  lenkung:      v => Math.round(v / 255 * 100) + ' %',
  lenkungStand: v => Math.round(v / 255 * 100) + ' %',
  ringHell:     v => Math.round(v / 255 * 100) + ' %',
  lichtHell:    v => Math.round(v / 255 * 100) + ' %',
  figurGroesse: v => v + ' %',
  lautstaerke:  v => v + ' %',
  crashG:       v => (+v).toFixed(1) + ' g  ' + t(v < 2 ? '(sehr empfindlich)' : v < 3 ? '(normal)' : v < 4.5 ? '(unempfindlich)' : '(nur heftige St\u00f6\u00dfe)'),
};
function zeigeEinst(e){
  einst = e;
  for (const k in anzeige){
    const r = document.getElementById('e-' + k);
    if (r && document.activeElement !== r) r.value = e[k];
    document.getElementById('v-' + k).textContent = anzeige[k](r ? r.value : e[k]);
  }
  document.getElementById('e-geradeaus').checked = !!e.geradeaus;
  document.getElementById('e-crash').checked = !!e.crash;
  for (const k of ['motorL', 'motorR', 'motorTausch', 'autoUpdate']) document.getElementById('e-' + k).checked = !!e[k];
  document.querySelectorAll('[data-toene]').forEach(b => b.classList.toggle('an', +b.dataset.toene === e.toene));
}
let speicherTimer = null;
async function speichere(teil){
  const st = document.getElementById('e-status');
  st.textContent = t('speichert...');
  try {
    const q = new URLSearchParams(teil).toString();
    zeigeEinst(await (await fetch('/einstellungen?' + q)).json());
    st.textContent = t('\u2713 gespeichert');
    clearTimeout(speicherTimer); speicherTimer = setTimeout(() => st.textContent = '', 2000);
  } catch(e){ st.textContent = t('Fehler beim Speichern'); }
}
for (const k in anzeige){
  const r = document.getElementById('e-' + k);
  r.addEventListener('input', () => document.getElementById('v-' + k).textContent = anzeige[k](r.value));
  r.addEventListener('change', () => speichere({[k]: r.value}));        // beim Loslassen speichern
}
document.getElementById('e-geradeaus').addEventListener('change', e => speichere({geradeaus: e.target.checked ? 1 : 0}));
document.getElementById('e-crash').addEventListener('change', e => speichere({crash: e.target.checked ? 1 : 0}));
// Motor-Schalter: Pfeil am Rad dreht sich kurz um = "Richtung umgedreht"
function pfeilDrehen(seite){
  const p = document.getElementById(seite ? 'pfeilR' : 'pfeilL');
  p.classList.remove('dreh'); void p.getBBox(); p.classList.add('dreh');
  setTimeout(() => p.classList.remove('dreh'), 1300);
}
document.getElementById('e-motorL').addEventListener('change', e => { speichere({motorL: e.target.checked ? 1 : 0}); pfeilDrehen(0); });
document.getElementById('e-motorR').addEventListener('change', e => { speichere({motorR: e.target.checked ? 1 : 0}); pfeilDrehen(1); });
document.getElementById('e-motorTausch').addEventListener('change', e => { speichere({motorTausch: e.target.checked ? 1 : 0}); pfeilDrehen(0); pfeilDrehen(1); });
document.querySelectorAll('[data-toene]').forEach(b => b.addEventListener('click', () => speichere({toene: b.dataset.toene})));
document.getElementById('e-standard').addEventListener('click', () => {
  if (confirm(t('Alle Einstellungen auf Standard zur\u00fccksetzen?'))) speichere({standard: 1});
});
(async () => { try { zeigeEinst(await (await fetch('/einstellungen')).json()); } catch(e){} })();

// ---------- Raeder in der Live-Ansicht drehen sich mit den echten Motorwerten ----------
let motL = 0, motR = 0, motTest = 0;
const profil = [document.getElementById('profilL'), document.getElementById('profilR')];
const glow = [document.querySelectorAll('.gl'), document.querySelectorAll('.gr')];
const versatz = [0, 0];
let letzteZeit = performance.now();
function raederSchleife(t){
  const dt = Math.min((t - letzteZeit) / 1000, 0.1); letzteZeit = t;
  [motL, motR].forEach((v, i) => {
    // vorwaerts: Profil oben auf dem Reifen laeuft nach vorne (in der Ansicht nach oben)
    versatz[i] = ((versatz[i] - v / 255 * 110 * dt) % 9 + 9) % 9;
    profil[i].setAttribute('patternTransform', 'translate(0 ' + versatz[i].toFixed(2) + ')');
    const st = Math.min(Math.abs(v) / 200, 1);
    glow[i].forEach(r => {
      r.style.opacity = Math.abs(v) > 8 ? (0.35 + 0.65 * st) : (motTest === i + 1 ? 0.6 : 0);
      r.style.stroke = v < -8 ? 'var(--warn)' : v > 8 ? 'var(--gut)' : 'var(--akzent)';
      r.style.filter = Math.abs(v) > 8 ? 'drop-shadow(0 0 4px ' + (v < 0 ? '#f0a030' : '#3ccf6e') + ')' : '';
    });
  });
  document.getElementById('pfeilL').classList.toggle('an', motTest === 1);
  document.getElementById('pfeilR').classList.toggle('an', motTest === 2);
  requestAnimationFrame(raederSchleife);
}
requestAnimationFrame(raederSchleife);

// ---------- Motortest-Assistent ----------
const mtBox = document.getElementById('mt-box');
function mtFrage(text, antworten){
  document.getElementById('mt-text').innerHTML = text;
  const a = document.getElementById('mt-antw'); a.innerHTML = '';
  antworten.forEach(([titel, fn, gut]) => {
    const b = document.createElement('button'); b.textContent = titel;
    if (gut) b.className = 'gut';
    b.addEventListener('click', fn); a.appendChild(b);
  });
}
async function mtUmstellen(teil){ await speichere(teil); }
function mtSeite(seite){
  aktion(34 + seite);
  mtFrage(t('mt.schritt' + seite), [
    [t('mt.vor' + seite), () => seite ? mtFertig() : mtSeite(1), true],
    [t('mt.rueck' + seite), async () => { await mtUmstellen(seite ? {motorR: einst.motorR ? 0 : 1} : {motorL: einst.motorL ? 0 : 1}); pfeilDrehen(seite); mtNochmal(seite, t('Richtung umgedreht.')); }],
    [t('mt.andere' + seite), async () => {
      if (seite) { mtFrage(t('mt.passtnicht'), [[t('Neu starten'), mtStart], [t('Schlie\u00dfen'), mtEnde]]); return; }
      await mtUmstellen({motorTausch: einst.motorTausch ? 0 : 1}); pfeilDrehen(0); pfeilDrehen(1); mtNochmal(seite, t('Links und rechts getauscht.')); }],
    [t('Nichts / nochmal zeigen'), () => mtSeite(seite)],
  ]);
}
function mtNochmal(seite, was){
  mtFrage('&#10004; ' + was + ' ' + t('mt.nochmal'), []);
  setTimeout(() => mtSeite(seite), 1200);
}
function mtFertig(){ mtFrage(t('mt.fertig'), [[t('Schlie\u00dfen'), mtEnde, true]]); }
function mtEnde(){ mtBox.hidden = true; }
function mtStart(){
  mtBox.hidden = false;
  document.getElementById('liveauto').scrollIntoView({behavior: 'smooth', block: 'start'});
  mtFrage(t('mt.los'), [[t('mt.start'), () => mtSeite(0), true], [t('Abbrechen'), mtEnde]]);
}
document.getElementById('b-mtest').addEventListener('click', mtStart);

// ---------- Auto-Update ----------
let updZustand = 0;
document.getElementById('e-autoUpdate').addEventListener('change', e => speichere({autoUpdate: e.target.checked ? 1 : 0}));
document.getElementById('upd-suchen').addEventListener('click', () => { fetch('/update?a=suchen').catch(() => {}); document.getElementById('upd-status').textContent = t('sucht\u2026'); });
document.getElementById('upd-install').addEventListener('click', () => {
  if (!confirm(t('Update jetzt installieren? Das Auto l\u00e4dt ca. 1 Minute und startet dann neu.'))) return;
  fetch('/update?a=installieren').catch(() => {});
  document.getElementById('upd-text').textContent = t('Das Auto l\u00e4dt das Update und startet neu \u2013 die Seite verbindet sich danach von selbst wieder.');
});
document.getElementById('upd-pille').addEventListener('click', () => { const d = document.getElementById('einst'); d.open = true; document.getElementById('upd-suchen').scrollIntoView({behavior: 'smooth', block: 'center'}); });
function zeigeUpdate(s){
  if (s.fw === undefined) return;
  updZustand = s.upd;
  document.getElementById('fw-ist').textContent = s.fw ? 'Build ' + s.fw : t('selbst gebaut');
  document.getElementById('fw-neu').textContent = s.fwneu ? 'Build ' + s.fwneu : '\u2013';
  const text = {0: '\u2013', 1: t('sucht\u2026'), 2: t('l\u00e4dt\u2026') + ' ' + s.updp + ' %', 3: t('\u2713 aktuell'), 4: t('nicht erreichbar'), 5: t('Update verf\u00fcgbar!')};
  document.getElementById('upd-status').textContent = text[s.upd] || '\u2013';
  document.getElementById('upd-install').hidden = s.upd !== 5;
  document.getElementById('upd-pille').hidden = s.upd !== 5;
  document.getElementById('version-build').textContent = s.fw ? ' \u00b7 Build ' + s.fw : '';
}

// ---------- Jukebox ----------
const LIEDER = [["\ud83e\uddf1", "Tetris-Melodie", "Korobeiniki \u00b7 russisches Volkslied"], ["\u26f0\ufe0f", "In der Halle des Bergk\u00f6nigs", "Edvard Grieg \u00b7 wird immer schneller"], ["\ud83c\udfb9", "F\u00fcr Elise", "Ludwig van Beethoven"], ["\ud83c\udf89", "Ode an die Freude", "Ludwig van Beethoven"], ["\ud83c\udfa9", "The Entertainer", "Scott Joplin \u00b7 Ragtime"], ["\ud83e\udd16", "Roboauto-Theme", "Eigene Komposition"], ["\ud83d\udc7e", "Boss-Level", "Eigene Komposition"], ["\ud83c\udfc6", "Level geschafft", "Eigene Komposition"], ["\ud83c\udf06", "Neon-Jagd", "Eigene Komposition \u00b7 C64-Stil"], ["\ud83c\udf0c", "Pixel-Galaxie", "Eigene Komposition \u00b7 episch"], ["\ud83c\udfce\ufe0f", "Turbo-Rennen", "Eigene Komposition \u00b7 mit Startampel"], ["\ud83c\udff9", "Wilhelm Tell", "Gioachino Rossini \u00b7 Galopp"], ["\ud83d\udc1d", "Hummelflug", "Nikolai Rimski-Korsakow \u00b7 rasend schnell"]];
let liedAktiv = -1, liedTon = 0;
function zeigeLieder(){
  const box = document.getElementById('lieder'); box.innerHTML = '';
  LIEDER.forEach(([ico, name, info], i) => {
    const b = document.createElement('button');
    b.className = 'lied' + (i === liedAktiv ? ' an' : ''); b.id = 'lied-' + i;
    b.innerHTML = '<i class="ico"></i><span><b></b><small></small></span>';
    b.querySelector('.ico').textContent = ico; b.querySelector('b').textContent = t(name); b.querySelector('small').textContent = t(info); b.title = t(name) + ' \u2013 ' + t(info);
    b.addEventListener('click', () => aktion(41 + i));
    box.appendChild(b);
  });
}
function zeigeMusik(s){
  if (s.lied === undefined) return;
  if (s.lied !== liedAktiv){
    liedAktiv = s.lied;
    LIEDER.forEach((_, i) => document.getElementById('lied-' + i).classList.toggle('an', i === liedAktiv));
    document.getElementById('lied-titel').textContent = liedAktiv >= 0 ? LIEDER[liedAktiv][0] + '  ' + t(LIEDER[liedAktiv][1]) : '\u2013';
  }
  document.getElementById('lied-balken').style.width = (liedAktiv >= 0 ? s.liedpos : 0) + '%';
  liedTon = s.ton || 0;
}
document.getElementById('lied-stopp').addEventListener('click', () => aktion(40));
// Visualizer: Balken huepfen zur aktuellen Tonhoehe
const vizHoehe = new Array(24).fill(0);
function vizSchleife(){
  const c = document.getElementById('viz'), g = c.getContext('2d'), B = c.width, H = c.height, n = vizHoehe.length;
  g.clearRect(0, 0, B, H);
  const zeit = performance.now() / 1000;
  let mitte = -1;
  if (liedAktiv >= 0 && liedTon > 0) mitte = ((12 * Math.log2(liedTon / 440) + 12) / 36) * (n - 1);
  for (let i = 0; i < n; i++){
    let ziel = 0.04;
    if (liedAktiv >= 0){
      ziel = 0.10 + 0.08 * (0.5 + 0.5 * Math.sin(zeit * 3 + i * 0.7));
      if (mitte >= 0) ziel += 0.85 * Math.exp(-Math.pow((i - mitte) / 2.2, 2));
    }
    vizHoehe[i] += (Math.min(ziel, 1) - vizHoehe[i]) * (ziel > vizHoehe[i] ? 0.45 : 0.10);
    const bw = B / n, h = Math.max(4, vizHoehe[i] * (H - 8)), x = i * bw + 3, y = H - h;
    const grad = g.createLinearGradient(0, H, 0, 0);
    grad.addColorStop(0, '#4f9cff'); grad.addColorStop(0.55, '#7c6cff'); grad.addColorStop(1, '#ff4fd8');
    g.fillStyle = grad; g.globalAlpha = liedAktiv >= 0 ? 1 : 0.35;
    g.beginPath(); g.roundRect ? g.roundRect(x, y, bw - 6, h, 4) : g.rect(x, y, bw - 6, h); g.fill();
  }
  g.globalAlpha = 1;
  requestAnimationFrame(vizSchleife);
}
requestAnimationFrame(vizSchleife);

// ---------- Sprache (Deutsch / English) - wird im Browser gemerkt ----------
const EN = {"keine Verbindung": "not connected", "verbinde...": "connecting...", "verbunden": "connected", "Akku": "Battery", "Motoren L / R": "Motors L / R", "Licht": "Lights", "Fernlicht": "High beam", "Warnblinker": "Hazards", "180\u00b0 Wende": "180\u00b0 turn", "\u25c0 Blinker": "\u25c0 Indicator", "Blinker \u25b6": "Indicator \u25b6", "Q halten": "hold Q", "E halten": "hold E", "Hupe": "Horn", "Sirene": "Siren", "H / Leertaste": "H / Space", "Tank": "Battery gauge", "WLAN": "Wi-Fi", "T halten": "hold T", "Normal": "Normal", "Polizei": "Police", "Team Blau": "Team Blue", "Team Gr\u00fcn": "Team Green", "Team Gelb": "Team Yellow", "Team Rot": "Team Red", "Live-Ansicht von oben \u00b7 Lichter und Ring leuchten wie am Auto": "Live top view \u00b7 lights and ring glow just like on the car", "vorne": "front", "\ud83d\udd27 Motortest-Assistent": "\ud83d\udd27 Motor test wizard", "Richtung": "Heading", "Neigung vorn/hinten": "Pitch (front/back)", "Neigung seitlich": "Roll (sideways)", "Motorleistung": "Motor power", "Akkuverlauf (letzte Stunde)": "Battery history (last hour)", "Figuren:": "Figures:", "\u25a2 Quadrat": "\u25a2 Square", "\u221e Acht": "\u221e Figure eight", "\u223f Slalom": "\u223f Slalom", "Fahrweg:": "Route:", "\u25cf Aufnahme": "\u25cf Record", "\u25b6 Abspielen": "\u25b6 Play", "Experiment:": "Experiment:", "\u2696 Balancieren": "\u2696 Balance", "Auto am Balancierpunkt halten, dann klicken. Nochmal klicken = aus.": "Hold the car at its balance point, then click. Click again = off.", "Modus wechseln: Knopf anklicken oder": "Change mode: click a button or press", "(n\u00e4chster Modus)": "(next mode)", "WLAN \u00e4ndern": "Change Wi-Fi", "\u2699 Einstellungen": "\u2699 Settings", "H\u00f6chstgeschwindigkeit": "Top speed", "Figurengr\u00f6\u00dfe": "Figure size", "Lenkung beim Fahren": "Steering while driving", "Drehen auf der Stelle": "Turning on the spot", "Helligkeit LED-Ring": "LED ring brightness", "Helligkeit Lichter": "Light brightness", "Crash-Empfindlichkeit": "Crash sensitivity", "Gyro-Geradeauslauf": "Gyro straight-line assist", "Crash- und Umkipp-Erkennung": "Crash and tip-over detection", "\ud83c\udfaf Gyro neu kalibrieren": "\ud83c\udfaf Recalibrate gyro", "Auto dazu auf den Boden stellen und 1 Sekunde nicht ber\u00fchren. Setzt auch den Kompass auf 0\u00b0.": "Place the car on the floor and don\u2019t touch it for 1 second. Also resets the compass to 0\u00b0.", "T\u00f6ne": "Sounds", "Alle": "All", "Nur wichtige": "Important only", "Stumm": "Mute", "\u2699\ufe0f Motoren \u2013 falls falsch angeschlossen": "\u2699\ufe0f Motors \u2013 if wired the wrong way", "Linke Seite umkehren": "Reverse left side", "Rechte Seite umkehren": "Reverse right side", "Links und rechts tauschen": "Swap left and right", "\ud83d\udd27 Motortest-Assistent starten": "\ud83d\udd27 Start motor test wizard", "Der Assistent dreht nacheinander kurz jede Seite und fragt, was passiert ist. Die richtigen Schalter stellt er dann selbst ein. Auto dazu am besten hochheben.": "The wizard briefly spins each side in turn and asks what happened. It then sets the right switches by itself. Best lift the car up for this.", "Standard wiederherstellen": "Restore defaults", "Software: Schritt": "Software: step", "Fahren:": "Drive:", "Pfeiltasten": "arrow keys", "oder den blauen Knopf ziehen. Loslassen = Stopp.": "or drag the blue knob. Release = stop.", "r\u00fcckw\u00e4rts": "reverse", "vorw\u00e4rts": "forward", "steht": "stopped", "sammelt Daten... (alle {0} s ein Punkt)": "collecting data... (one point every {0} s)", "jetzt": "now", "vor {0} min": "{0} min ago", "(sehr empfindlich)": "(very sensitive)", "(normal)": "(normal)", "(unempfindlich)": "(less sensitive)", "(nur heftige St\u00f6\u00dfe)": "(hard hits only)", "speichert...": "saving...", "\u2713 gespeichert": "\u2713 saved", "Fehler beim Speichern": "Error while saving", "Alle Einstellungen auf Standard zur\u00fccksetzen?": "Reset all settings to defaults?", "Messwert {0}": "Raw value {0}", "nimmt auf...": "recording...", "gespeichert: {0} s": "saved: {0} s", "noch keine Aufnahme": "no recording yet", "rechts": "right", "links": "left", "dreht {0}\u00b0/s": "turning {0}\u00b0/s", "mt.los": "Lift the car up or place it so the wheels can spin freely. Then let\u2019s start.", "mt.start": "Let\u2019s go", "Abbrechen": "Cancel", "mt.schritt0": "Step 1 of 2: The car now spins the <b>LEFT wheels forward for 1.5 seconds</b> &ndash; as shown by the green arrow above. What did you see?", "mt.schritt1": "Step 2 of 2: The car now spins the <b>RIGHT wheels forward for 1.5 seconds</b> &ndash; as shown by the green arrow above. What did you see?", "mt.vor0": "Left wheels forward \u2713", "mt.vor1": "Right wheels forward \u2713", "mt.rueck0": "Left wheels backward", "mt.rueck1": "Right wheels backward", "mt.andere0": "The right wheels turned", "mt.andere1": "The left wheels turned", "Nichts / nochmal zeigen": "Nothing / show again", "mt.passtnicht": "Hmm, that doesn\u2019t match step 1. Please check the wiring and restart the test.", "Neu starten": "Restart", "Schlie\u00dfen": "Close", "Richtung umgedreht.": "Direction reversed.", "Links und rechts getauscht.": "Left and right swapped.", "mt.nochmal": "Testing again in a moment&hellip;", "mt.fertig": "&#127881; <b>Done!</b> Both sides spin the right way. The setting is saved in the car. To check, just drive forward."};
const DE_TEXT = {"mt.los": "Heb das Auto hoch oder stell es so hin, dass sich die R&auml;der frei drehen k&ouml;nnen. Dann geht es los.", "mt.start": "Los geht\u2019s", "mt.schritt0": "Schritt 1 von 2: Das Auto dreht jetzt <b>1,5 Sekunden die LINKEN R&auml;der vorw&auml;rts</b> &ndash; so wie der gr&uuml;ne Pfeil oben zeigt. Was hast du gesehen?", "mt.schritt1": "Schritt 2 von 2: Das Auto dreht jetzt <b>1,5 Sekunden die RECHTEN R&auml;der vorw&auml;rts</b> &ndash; so wie der gr&uuml;ne Pfeil oben zeigt. Was hast du gesehen?", "mt.vor0": "Linke R\u00e4der vorw\u00e4rts \u2713", "mt.vor1": "Rechte R\u00e4der vorw\u00e4rts \u2713", "mt.rueck0": "Linke R\u00e4der r\u00fcckw\u00e4rts", "mt.rueck1": "Rechte R\u00e4der r\u00fcckw\u00e4rts", "mt.andere0": "Die rechten R\u00e4der haben sich gedreht", "mt.andere1": "Die linken R\u00e4der haben sich gedreht", "mt.passtnicht": "Hmm, das passt nicht zu Schritt 1. Bitte die Verkabelung pr&uuml;fen und den Test neu starten.", "mt.nochmal": "Gleich wird nochmal getestet&hellip;", "mt.fertig": "&#127881; <b>Fertig!</b> Beide Seiten drehen richtig herum. Die Einstellung ist im Auto gespeichert. Zum Pr&uuml;fen einfach mal vorw&auml;rts fahren."};
Object.assign(EN, {"Steuerung": "Controls", "Loslassen = Stopp": "Release = stop", "Beleuchtung": "Lighting", "Signale": "Signals", "Man\u00f6ver & Anzeige": "Manoeuvres & display", "\ud83c\udfaf Kalibrieren": "\ud83c\udfaf Calibrate", "Gyro": "Gyro", "Modus \u00b7": "Mode \u00b7", "= n\u00e4chster": "= next", "Live": "Live", "Programme": "Programs", "\u2728 Figuren": "\u2728 Figures", "Nochmal klicken oder lenken = Stopp.": "Click again or steer = stop.", "\ud83c\udfac Fahrweg": "\ud83c\udfac Route", "\ud83e\uddea Experiment": "\ud83e\uddea Experiment", "Einstellungen": "Settings", "\ud83c\udfce\ufe0f Fahren": "\ud83c\udfce\ufe0f Driving", "\ud83d\udee1\ufe0f Sicherheit": "\ud83d\udee1\ufe0f Safety", "Gyro kalibrieren: Auto dazu auf den Boden stellen und 1 Sekunde nicht ber\u00fchren. Setzt auch den Kompass auf 0\u00b0.": "Calibrating the gyro: place the car on the floor and don\u2019t touch it for 1 second. Also resets the compass to 0\u00b0.", "\ud83d\udca1 Licht & Ton": "\ud83d\udca1 Lights & sound", "\u2699\ufe0f Motoren": "\u2699\ufe0f Motors", "\ud83d\udda5\ufe0f System": "\ud83d\udda5\ufe0f System", "\ud83d\udcf6 WLAN \u00e4ndern": "\ud83d\udcf6 Change Wi-Fi", "\u21ba Standard wiederherstellen": "\u21ba Restore defaults", "Tastatur:": "Keyboard:", "Wende": "Turn", "Blinker": "Indicators", "Modus": "Mode"});
Object.assign(EN, {'Tempo, Lenkung, Sicherheit, Licht & Ton, Motoren, WLAN': 'Speed, steering, safety, lights & sound, motors, Wi-Fi'});
Object.assign(EN, {"Steuerung": "Controls", "Programme": "Programs", "\ud83c\udfb5 Musik": "\ud83c\udfb5 Music", "Jukebox": "Jukebox", "Jetzt l\u00e4uft": "Now playing", "\u25a0 Stopp": "\u25a0 Stop", "Spielt \u00fcber den Buzzer im Auto, der LED-Ring macht die Lichtshow dazu. Nochmal auf das Lied klicken = Stopp.": "Plays through the buzzer in the car, the LED ring adds a light show. Click the song again = stop.", "Tetris-Melodie": "Tetris theme", "Korobeiniki \u00b7 russisches Volkslied": "Korobeiniki \u00b7 Russian folk song", "In der Halle des Bergk\u00f6nigs": "In the Hall of the Mountain King", "Edvard Grieg \u00b7 wird immer schneller": "Edvard Grieg \u00b7 keeps getting faster", "F\u00fcr Elise": "F\u00fcr Elise", "Ode an die Freude": "Ode to Joy", "Eigene Komposition": "Original composition", "Level geschafft": "Level complete", "nichts": "nothing", "\u2013": "\u2013"});
Object.assign(EN, {'\ud83d\udd0a Lautst\u00e4rke': '\ud83d\udd0a Volume'});
Object.assign(EN, {"\ud83d\udd04 Updates": "\ud83d\udd04 Updates", "Installiert": "Installed", "Auf GitHub": "On GitHub", "Status": "Status", "Automatisch aktualisieren": "Update automatically", "\ud83d\udd0d Nach Updates suchen": "\ud83d\udd0d Check for updates", "\u2b07\ufe0f Jetzt installieren": "\u2b07\ufe0f Install now", "Update": "Update", "Neue Versionen kommen automatisch von GitHub. Installiert wird nur, wenn das Auto 2 Minuten unbenutzt steht und der Akku reicht. Startet eine neue Version nicht sauber, schaltet das Auto von selbst auf die alte zur\u00fcck.": "New versions come automatically from GitHub. They are only installed when the car has been idle for 2 minutes and the battery is good. If a new version does not start properly, the car automatically switches back to the old one.", "sucht\u2026": "checking\u2026", "l\u00e4dt\u2026": "downloading\u2026", "\u2713 aktuell": "\u2713 up to date", "nicht erreichbar": "not reachable", "Update verf\u00fcgbar!": "Update available!", "selbst gebaut": "self-built", "Update jetzt installieren? Das Auto l\u00e4dt ca. 1 Minute und startet dann neu.": "Install the update now? The car downloads for about 1 minute and then restarts.", "Das Auto l\u00e4dt das Update und startet neu \u2013 die Seite verbindet sich danach von selbst wieder.": "The car is downloading the update and will restart \u2013 the page reconnects by itself afterwards."});
Object.assign(EN, {"Neon-Jagd": "Neon Chase", "Eigene Komposition \u00b7 C64-Stil": "Original composition \u00b7 C64 style", "Pixel-Galaxie": "Pixel Galaxy", "Eigene Komposition \u00b7 episch": "Original composition \u00b7 epic", "Turbo-Rennen": "Turbo Race", "Eigene Komposition \u00b7 mit Startampel": "Original composition \u00b7 with start lights", "Wilhelm Tell": "William Tell", "Gioachino Rossini \u00b7 Galopp": "Gioachino Rossini \u00b7 Gallop", "Hummelflug": "Flight of the Bumblebee", "Nikolai Rimski-Korsakow \u00b7 rasend schnell": "Nikolai Rimsky-Korsakov \u00b7 blazing fast"});
let sprache = 'de';
try { sprache = localStorage.getItem('sprache') || (/^de/i.test(navigator.language || 'de') ? 'de' : 'en'); } catch(e){}
// t('Text', a, b) -> Text in der gewaehlten Sprache, {0} {1} werden ersetzt
function t(s, ...w){
  let r = sprache === 'en' ? (EN[s] || s) : (DE_TEXT[s] || s);
  w.forEach((v, i) => r = r.replace('{' + i + '}', v));
  return r;
}
function uebersetzeSeite(){
  document.documentElement.lang = sprache;
  const gang = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
  let n;
  while ((n = gang.nextNode())){
    if (n.parentNode.closest('script,style,#mt-box')) continue;
    if (n.deText === undefined) n.deText = n.nodeValue;
    const roh = n.deText, kern = roh.trim();
    if (!kern || !(kern in EN)) continue;
    n.nodeValue = roh.replace(kern, t(kern));
  }
  document.getElementById('mt-titel').textContent = t('\ud83d\udd27 Motortest-Assistent');
  document.getElementById('sprache-name').textContent = sprache === 'en' ? 'Deutsch' : 'English';
  if (einst.tempo !== undefined) zeigeEinst(einst);
  zeigeLieder();
  if (liedAktiv >= 0) document.getElementById('lied-titel').textContent = LIEDER[liedAktiv][0] + '  ' + t(LIEDER[liedAktiv][1]);
}
document.getElementById('b-sprache').addEventListener('click', () => {
  sprache = sprache === 'en' ? 'de' : 'en';
  try { localStorage.setItem('sprache', sprache); } catch(e){}
  uebersetzeSeite();
  zeichneAkku(true);
  if (!mtBox.hidden) mtEnde();
});
uebersetzeSeite();

document.getElementById('b-einst').addEventListener('click', () => {
  const d = document.getElementById('einst');
  d.open = true;
  d.scrollIntoView({behavior: 'smooth', block: 'start'});
});

// ---------- Status 2x pro Sekunde (gleichzeitig Lebenszeichen ans Auto) ----------
async function status(){
  try{
    const r = await fetch('/status'), s = await r.json();
    const akkuText = s.usb ? 'USB' : s.prozent + ' %' + (s.eco ? ' ECO' : '');
    document.getElementById('akku').textContent = akkuText;
    document.getElementById('akku').title = t('Messwert {0}', s.akku);
    const balken = document.getElementById('akkubalken');
    balken.style.width = (s.usb ? 100 : s.prozent) + '%';
    balken.style.background = s.usb ? 'var(--akzent)' : s.prozent > 60 ? 'var(--gut)' : s.prozent > 25 ? 'var(--warn)' : 'var(--rot)';
    document.getElementById('mot').textContent  = s.links + ' / ' + s.rechts;
    document.getElementById('wlan').textContent = s.rssi + ' dBm';
    document.getElementById('b-licht').classList.toggle('an', !!s.licht);
    document.getElementById('b-fern').classList.toggle('an', !!s.fern);
    document.getElementById('b-warn').classList.toggle('an', !!s.warn);
    document.getElementById('b-wende').classList.toggle('an', !!s.wende);
    for (let m = 0; m < 6; m++) document.getElementById('m-' + m).classList.toggle('an', s.modus === m);
    for (let f = 0; f < 3; f++) document.getElementById('f-' + f).classList.toggle('an', s.figur === f);
    document.getElementById('b-balance').classList.toggle('an', !!s.balance);
    document.getElementById('r-rec').classList.toggle('an', !!s.rec);
    document.getElementById('r-play').classList.toggle('an', !!s.play);
    document.getElementById('r-info').textContent =
      s.rec ? t('nimmt auf...') : s.play ? s.playpos.toFixed(1) + ' / ' + s.reclen.toFixed(1) + ' s'
            : (s.reclen > 0 ? t('gespeichert: {0} s', s.reclen.toFixed(1)) : t('noch keine Aufnahme'));
    document.getElementById('b-hupe').firstChild.textContent = t(s.modus === 1 ? 'Sirene' : 'Hupe');
    document.getElementById('b-kal').classList.toggle('an', !!s.kal);
    zeichneKompass(s.kurs);
    document.getElementById('t-kurs').textContent = Math.round(s.kurs) + '°';
    zeichneNeigung('g-nicken', s.nicken, false);
    zeigeLiveAuto(s);
    motL = s.links; motR = s.rechts; motTest = s.mt || 0;
    zeigeMusik(s);
    zeigeUpdate(s);
    document.getElementById('t-nicken').textContent = (s.nicken > 0 ? '+' : '') + s.nicken.toFixed(0) + '°';
    // Heckansicht: nach rechts kippen (Sensorwert positiv) = rechte Seite runter -> Zeichnung spiegeln
    zeichneNeigung('g-rollen', -s.rollen, true);
    const rl = Math.round(s.rollen);
    document.getElementById('t-rollen').textContent = rl === 0 ? '0°' : Math.abs(rl) + '° ' + t(rl > 0 ? 'rechts' : 'links');
    const tempo = Math.round((s.links + s.rechts) / 2 / (einst.tempo || 128) * 100);
    zeichneTempo(Math.max(-100, Math.min(100, tempo)));
    document.getElementById('t-tempo').textContent = Math.abs(s.dreh) > 20 ? t('dreht {0}°/s', Math.round(Math.abs(s.dreh))) : '';
    document.getElementById('punkt').style.background = 'var(--gut)';
    document.getElementById('verb').textContent = t('verbunden');
  } catch(e){
    document.getElementById('punkt').style.background = 'var(--rot)';
    document.getElementById('verb').textContent = t('keine Verbindung');
  }
  setTimeout(status, 250);   // 4x pro Sekunde - fluessigere Live-Anzeigen
}
status();
</script>
</body></html>)==";
