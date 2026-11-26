// =============================================================================
// Room Labs Audio — bloc « Licence & à propos » commun aux interfaces web des plugins.
//   RLLicenseUI.mount(conteneur, { call, native, product, version })
// Affiche : statut de la licence, boutons Activer / Hors ligne / Gérer (désactiver cet ordinateur),
// liens du site, © et ℗. Les écrans d'activation sont ceux du module officiel Moonbase (natifs).
// =============================================================================
(function () {
  const css = `
  .rlLic{margin-top:16px;padding-top:14px;border-top:1px solid rgba(255,255,255,.14);font-family:inherit}
  .rlLic h4{margin:0 0 10px;font-weight:700;font-size:15px;letter-spacing:2px;text-transform:uppercase;opacity:.9}
  .rlLic .st{display:flex;align-items:center;gap:10px;font-weight:600;font-size:15px}
  .rlLic .st i{width:11px;height:11px;border-radius:50%;flex:none;box-shadow:0 0 8px currentColor}
  .rlLic .bt{display:flex;flex-wrap:wrap;gap:8px;margin-top:10px}
  .rlLic button{height:36px;padding:0 14px;border-radius:18px;border:1px solid rgba(255,255,255,.22);background:rgba(255,255,255,.07);color:inherit;font-weight:600;font-size:14px;cursor:pointer}
  .rlLic button:hover{background:rgba(255,255,255,.14)}
  .rlLic button:disabled{opacity:.45;cursor:default}
  .rlLic .nt{font-weight:500;font-size:13px;opacity:.75;margin-top:8px;line-height:1.45}
  .rlLic .lk{display:flex;flex-direction:column;gap:4px;margin-top:12px}
  .rlLic a{font-weight:600;font-size:14px;color:inherit;text-decoration:underline;text-underline-offset:3px;cursor:pointer;opacity:.95}
  .rlLic .cp{font-weight:500;font-size:13px;opacity:.75;margin-top:10px}`;
  function mount(box, o) {
    if (!document.getElementById("rlLicCss")) { const s = document.createElement("style"); s.id = "rlLicCss"; s.textContent = css; document.head.appendChild(s); }
    const el = document.createElement("div"); el.className = "rlLic";
    el.innerHTML = `<h4>Licence</h4><div class="st"><i></i><span class="tx">—</span></div>
      <div class="bt"><button data-a="act">Activer</button><button data-a="off">Activer hors ligne (fichier)</button><button data-a="man">Gérer / désactiver cet ordinateur</button></div>
      <div class="nt"></div>
      <div class="lk"><a data-u="https://www.7throomlabsaudio.com">www.7throomlabsaudio.com</a><a data-u="https://roomlabsaudio.com/plugins">roomlabsaudio.com/plugins</a></div>
      <div class="cp">${o.product || ""} ${o.version ? "v" + o.version : ""} · © 2026 Room Labs Audio · ℗ 2026 Room Labs Audio</div>`;
    box.appendChild(el);
    const open = u => { if (o.native) o.call("openUrl", u); else window.open(u, "_blank"); };
    el.querySelectorAll("a").forEach(a => a.onclick = e => { e.preventDefault(); open(a.dataset.u); });
    el.querySelectorAll("button").forEach(b => b.onclick = () => { if (o.native) o.call("licensePanel", b.dataset.a); });
    function show(s) {
      s = s || { configured: false, licensed: true, maxDevices: 3, graceDays: 14 };
      const dot = el.querySelector(".st i"), tx = el.querySelector(".tx"), nt = el.querySelector(".nt");
      if (!s.configured) { dot.style.color = "#f0c040"; dot.style.background = "#f0c040"; tx.textContent = "Version de développement — licence pas encore activée"; }
      else if (s.licensed) { dot.style.color = "#47d17a"; dot.style.background = "#47d17a"; tx.textContent = "Licence active sur cet ordinateur"; }
      else { dot.style.color = "#ff5a4e"; dot.style.background = "#ff5a4e"; tx.textContent = "Licence non activée"; }
      el.querySelectorAll("button").forEach(b => b.disabled = !s.configured || !o.native);
      nt.textContent = `Jusqu'à ${s.maxDevices || 3} ordinateurs par licence. Fonctionne hors ligne pendant ${s.graceDays || 14} jours après la dernière vérification en ligne.`;
    }
    if (o.native) { o.call("license").then(show); if (window.__JUCE__) window.__JUCE__.backend.addEventListener("license", show); } else show();
    return { refresh: () => o.native ? o.call("license").then(show) : show() };
  }
  window.RLLicenseUI = { mount };
})();
