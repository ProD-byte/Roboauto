// Roboauto-App: laeuft VOR dem Skript der Weboberflaeche.
// Die Webseite fragt wie im Browser fetch('/status'), fetch('/fahr?...') usw.
// Hier wird das an die App (window.Roboauto) umgeleitet - die schickt es per Bluetooth oder WLAN ans Auto.
(function () {
  if (!window.Roboauto) return;

  let naechsteNr = 1;
  const offen = {};

  // Antwort vom Auto (ruft die App auf)
  window.__roboAntwort = function (nr, ok, text) {
    const p = offen[nr];
    if (!p) return;
    delete offen[nr];
    clearTimeout(p.uhr);
    if (ok) p.ja(new Response(text, {headers: {'Content-Type': text.charAt(0) === '{' ? 'application/json' : 'text/plain'}}));
    else p.nein(new Error(text));
  };

  const echtesFetch = window.fetch.bind(window);
  window.fetch = function (url, optionen) {
    url = String(url);
    if (url.charAt(0) !== '/') return echtesFetch(url, optionen);
    // Fahrbefehle: schnell und ohne Antwort
    if (url.indexOf('/fahr') === 0) {
      Roboauto.senden(url);
      return Promise.resolve(new Response('ok'));
    }
    return new Promise(function (ja, nein) {
      const nr = naechsteNr++;
      const uhr = setTimeout(function () {
        if (offen[nr]) { delete offen[nr]; nein(new Error('keine Antwort')); }
      }, 3000);
      offen[nr] = {ja: ja, nein: nein, uhr: uhr};
      Roboauto.anfrage(nr, url);
    });
  };

  // Kleine Pille in der Kopfleiste: womit ist die App verbunden? Antippen = Verbindung wechseln
  document.addEventListener('DOMContentLoaded', function () {
    const pillen = document.querySelector('.pillen');
    if (!pillen) return;
    const ble = Roboauto.art() === 'ble';
    const p = document.createElement('div');
    p.className = 'pille';
    p.style.cursor = 'pointer';
    p.title = 'Verbindung wechseln';
    p.innerHTML = ble
      ? '<svg width="12" height="16" viewBox="0 0 12 18" fill="none" stroke="#4f9cff" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M1 5l9 8-4.5 4V1L10 5 1 13"/></svg><b>Bluetooth</b>'
      : '<span>&#128246;</span><b>WLAN</b>';
    p.innerHTML += '<span class="t">&#8644;</span>';
    p.addEventListener('click', function () { Roboauto.trennen(); });
    pillen.insertBefore(p, pillen.firstChild);
  });
})();
