
#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"WFHTML(<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WearFall | Dashboard</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
<aside class="sidebar">
  <div class="brand">◉ <b>Wear<span>Fall</span></b></div>
  <nav>
    <button class="nav active" data-page="dashboard">⌂ Dashboard</button>
    <button class="nav" data-page="usuario">● Usuário</button>
    <button class="nav" data-page="alertas">⚠ Alertas</button>
    <button class="nav" data-page="historico">◷ Histórico</button>
  </nav>
  <div class="account">👤 <div><b>Responsável</b><small>responsavel@email.com</small></div></div>
</aside>

<main>
<header><div><h1 id="title">Dashboard</h1><p>Acompanhamento do dispositivo WearFall</p></div>
<div id="connection" class="online offline">● ESP32 desconectado</div></header>

<section id="dashboard" class="page">
  <div class="row">
    <article class="card"><h3>USUÁRIO MONITORADO</h3>
      <div class="user"><div class="avatar">👵</div><div><h2>Maria Francisca</h2><p>72 anos</p><p>Dispositivo: <b id="deviceConnection">Desconectado</b></p><p>Bateria: <b id="battery">Não informada</b></p></div></div>
    </article>
    <article class="card"><h3>MONITORAMENTO</h3>
      <div class="monitor"><strong id="monitorIcon">…</strong><div><h2 id="monitorTitle">Aguardando ESP32</h2><p id="monitorDescription">Conecte-se à rede WearFall_AP para receber os dados.</p></div></div>
      <hr><p>Última atualização recebida: <b id="clock">Aguardando dados</b></p>
    </article>
  </div>

  <div class="row">
    <article class="card"><h3>STATUS ATUAL</h3>
      <div id="status" class="status neutral"><span>…</span><div><h2>SEM DADOS</h2><p>Aguardando resposta do dispositivo.</p></div></div>
    </article>
    <article class="card"><h3>ÚLTIMO ALERTA</h3>
      <div id="lastAlert" class="alert empty"><h2>Aguardando dados</h2><p>O último alerta aparecerá após a conexão.</p></div>
    </article>
  </div>

  <div class="row bottom">
    <article class="card"><h3>HISTÓRICO DE EVENTOS</h3>
      <table><thead><tr><th>Tempo desde início</th><th>Evento</th><th>Status</th><th>Detalhes</th></tr></thead>
      <tbody id="events"></tbody></table>
    </article>
    <article class="card actions"><h3>AÇÕES RÁPIDAS</h3>
      <button id="refresh" class="primary">↻ Atualizar agora</button>
      <button id="cancel" disabled>✓ Cancelar alerta no ESP32</button>
      <p class="action-note">Conecte-se ao Wi-Fi WearFall_AP e mantenha esta página aberta. O cancelamento só é habilitado quando o ESP32 informa um alerta.</p>
    </article>
  </div>
</section>

<section id="usuario" class="page hidden"><article class="card large"><h3>USUÁRIO</h3><h2>Maria Francisca</h2><p>72 anos</p><p>Dispositivo: <b id="deviceConnectionUser">Desconectado</b></p><p>Monitoramento: <b id="monitorUser">Aguardando dados</b></p></article></section>
<section id="alertas" class="page hidden"><article class="card large"><h3>ALERTAS</h3><div id="alertPage" class="alert empty"><h2>Aguardando dados</h2><p>Os alertas de possíveis quedas aparecerão aqui.</p></div></article></section>
<section id="historico" class="page hidden"><article class="card large"><h3>HISTÓRICO</h3><table><thead><tr><th>Tempo desde início</th><th>Evento</th><th>Status</th><th>Detalhes</th></tr></thead><tbody id="eventsFull"></tbody></table></article></section>

<footer>WearFall © 2026 — Protótipo acadêmico</footer>
</main>
<script src="script.js"></script>
</body>
</html>)WFHTML";

const char DASHBOARD_CSS[] PROGMEM = R"WFCSS(*{box-sizing:border-box;margin:0;padding:0}body{font-family:Arial,sans-serif;background:#f5f7fa;color:#182230}.sidebar{position:fixed;width:240px;height:100vh;background:#071a33;color:white;padding:28px 16px;display:flex;flex-direction:column}.brand{font-size:27px;padding:0 10px 35px}.brand span{color:#69a9ff}nav{display:grid;gap:8px}.nav{border:0;background:transparent;color:#d9e4f3;text-align:left;padding:14px;border-radius:9px;font-size:15px;cursor:pointer}.nav.active,.nav:hover{background:#1167d8;color:white}.account{margin-top:auto;background:#10294a;padding:15px;border-radius:10px;display:flex;gap:10px}.account small{display:block;color:#a9bad0;margin-top:4px;font-size:11px}main{margin-left:240px;padding:28px 34px;min-height:100vh}header{display:flex;justify-content:space-between;align-items:center;margin-bottom:25px}h1{font-size:30px}header p{color:#687587;margin-top:5px}.online{font-weight:bold;color:#1da957}.row{display:grid;grid-template-columns:1fr 1fr;gap:18px;margin-bottom:18px}.bottom{grid-template-columns:2fr 1fr}.card{background:white;border:1px solid #dfe5ec;border-radius:11px;padding:21px;box-shadow:0 2px 8px #00000008}.card h3{color:#1167d8;font-size:14px;margin-bottom:17px}.user,.monitor{display:flex;align-items:center;gap:18px}.avatar{width:80px;height:80px;border-radius:50%;background:#edf2f8;display:grid;place-items:center;font-size:40px}.card p{color:#687587;margin-top:6px}.green{color:#1da957}.monitor>strong{font-size:45px;color:#1da957;background:#dff6e7;padding:8px 20px;border-radius:15px}hr{border:0;border-top:1px solid #dfe5ec;margin:18px 0}.status,.alert{min-height:110px;border-radius:9px;padding:20px;display:flex;align-items:center;gap:15px}.normal{background:#edf9f1;border:1px solid #bfe9cc}.normal>span{font-size:38px;color:#1da957}.warning{background:#fff0f0!important;border:1px solid #f2bcbc!important}.warning>span,.warning h2{color:#d82f2f}.alert.empty{background:#f8fafc;border:1px solid #dfe5ec}.alert.active{background:#fff0f0;border:1px solid #f2bcbc}.alert.active h2{color:#d82f2f}table{width:100%;border-collapse:collapse;font-size:13px}th,td{text-align:left;padding:12px;border-bottom:1px solid #dfe5ec}th{background:#f7f9fc}.tag{font-weight:bold}.ok{color:#1da957}.cancel{color:#e68a13}.actions{display:flex;flex-direction:column;gap:10px}.actions button{padding:13px;border-radius:8px;border:1px solid #bdc8d5;background:white;cursor:pointer;font-weight:bold;text-align:left}.actions .primary{background:#1167d8;color:white;border-color:#1167d8}.large{min-height:250px}.hidden{display:none}footer{text-align:center;color:#7b8796;font-size:12px;padding:14px}@media(max-width:900px){.sidebar{width:190px}.row{grid-template-columns:1fr}.bottom{grid-template-columns:1fr}main{margin-left:190px;padding:20px}}@media(max-width:600px){.sidebar{position:static;width:100%;height:auto}.account{margin-top:20px}main{margin-left:0;padding:16px}header{flex-direction:column;align-items:flex-start;gap:10px}}

.online.offline{color:#b45309}
.online.connected{color:#1da957}
.neutral{background:#f5f7fa;border:1px solid #dfe5ec}
.neutral>span{font-size:38px;color:#687587}
.actions button:disabled{opacity:.5;cursor:not-allowed}
.action-note{font-size:12px;line-height:1.5}
.error-text{color:#b45309!important}

.warning-text{color:#d82f2f}
)WFCSS";

const char DASHBOARD_JS[] PROGMEM = R"WFJS(


const API_BASE = location.protocol === "file:" ? "http://192.168.4.1" : location.origin;
const POLL_MS = 2000;
const TIMEOUT_MS = 1800;

const navs = document.querySelectorAll(".nav");
const pages = document.querySelectorAll(".page");
const title = document.getElementById("title");
navs.forEach(n => n.addEventListener("click", () => {
  navs.forEach(x => x.classList.remove("active"));
  n.classList.add("active");
  pages.forEach(p => p.classList.add("hidden"));
  document.getElementById(n.dataset.page).classList.remove("hidden");
  title.textContent = n.textContent.trim();
}));

let online = false;
let lastStatus = null;
let lastHistory = [];
let busy = false;

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}
function formatUptime(ms) {
  if (!Number.isFinite(Number(ms))) return "Tempo não informado";
  const s = Math.floor(Number(ms) / 1000);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
  return `+${h}h ${String(m).padStart(2,"0")}min (desde inicialização)`;
}
function eventStatus(event) {
  if (event.cancelado) return "CANCELADO";
  if (event.quedaConfirmada ?? event.confirmada) return "CONFIRMADA";
  return "DESCARTADA";
}
function showHistory(history) {
  const rows = history.map(event => {
    const tr = document.createElement("tr");
    const status = eventStatus(event);
    const fields = [
      formatUptime(event.tempoMs ?? event.tempo ?? event.millis),
      status === "CONFIRMADA" ? "Queda confirmada" : status === "CANCELADO" ? "Alerta cancelado" : "Evento descartado",
      status,
      `Rotação: ${event.rotacao ? "sim" : "não"} · Postura: ${event.postura ? "sim" : "não"} · Imobilidade: ${event.imobilidade ? "sim" : "não"}`
    ];
    fields.forEach((value, i) => {
      const td = document.createElement("td");
      if (i === 2) {
        const b = document.createElement("b");
        b.className = status === "CONFIRMADA" ? "warning-text" : status === "CANCELADO" ? "cancel" : "ok";
        b.textContent = value;
        td.appendChild(b);
      } else td.textContent = value;
      tr.appendChild(td);
    });
    return tr;
  });
  for (const id of ["events","eventsFull"]) {
    const tbody = document.getElementById(id);
    tbody.replaceChildren(...rows.map(row => row.cloneNode(true)));
    if (!rows.length) {
      const tr = document.createElement("tr");
      const td = document.createElement("td");
      td.colSpan = 4;
      td.textContent = online ? "Nenhum evento registrado no ESP32." : "Aguardando conexão com o ESP32.";
      tr.appendChild(td);
      tbody.appendChild(tr);
    }
  }
}
function setPanel(id, className, symbol, heading, description) {
  const el = document.getElementById(id);
  el.className = className;
  el.replaceChildren();
  if (symbol) {
    const span = document.createElement("span");
    span.textContent = symbol;
    el.appendChild(span);
  }
  const div = document.createElement("div");
  const h2 = document.createElement("h2");
  h2.textContent = heading;
  const p = document.createElement("p");
  p.textContent = description;
  div.append(h2, p);
  el.appendChild(div);
}
function updateConnection(isConnected) {
  online = isConnected;
  const el = document.getElementById("connection");
  el.className = `online ${isConnected ? "connected" : "offline"}`;
  el.textContent = isConnected ? "● ESP32 conectado" : "● ESP32 desconectado";
  setText("deviceConnection", isConnected ? "Conectado" : "Desconectado");
  setText("deviceConnectionUser", isConnected ? "Conectado" : "Desconectado");
  setText("monitorUser", isConnected ? "Ativo" : "Sem comunicação");
  if (!isConnected) {
    lastStatus = null;
    setText("monitorIcon", "…");
    setText("monitorTitle", "Sem comunicação");
    setText("monitorDescription", "Verifique a rede WearFall_AP e o endereço do ESP32.");
    document.getElementById("cancel").disabled = true;
    setPanel("status", "status neutral", "…", "SEM DADOS", "Não é possível verificar o estado atual do dispositivo.");
    setPanel("alertPage", "alert empty", null, "Sem comunicação", "Reconecte o ESP32 para verificar alertas.");
  }
}
function updateStatus(data) {
  lastStatus = data;
  const raw = String(data.estado ?? "").toUpperCase();
  const alert = data.alerta === true || raw === "ALERTA";
  setText("clock", new Date().toLocaleTimeString("pt-BR"));
  setText("battery", data.bateria === null || data.bateria === undefined ? "Não medida" : `${data.bateria}%`);
  setText("monitorIcon", alert ? "!" : "✓");
  setText("monitorTitle", alert ? "Alerta ativo" : "Monitoramento ativo");
  setText("monitorDescription", alert ? "O dispositivo identificou uma possível queda." : `Estado do ESP32: ${raw || "MONITORANDO"}`);
  document.getElementById("cancel").disabled = !alert;
  setPanel("status", alert ? "status warning" : "status normal", alert ? "⚠" : "✓",
    alert ? "POSSÍVEL QUEDA" : raw || "MONITORANDO",
    alert ? "Verifique a situação da pessoa monitorada." : "Sem alerta ativo informado pelo dispositivo.");
  setPanel("alertPage", alert ? "alert active" : "alert empty", null,
    alert ? "⚠ POSSÍVEL QUEDA DETECTADA" : "Nenhum alerta ativo",
    alert ? "Verifique a situação da pessoa monitorada." : "O dispositivo não informa alerta ativo.");
}
function updateLastAlert() {
  const recent = lastHistory.find(e => e.quedaConfirmada === true || e.confirmada === true);
  setPanel("lastAlert", "alert empty", null,
    recent ? "Última queda confirmada" : "Nenhuma queda confirmada no histórico",
    recent ? formatUptime(recent.tempoMs ?? recent.tempo ?? recent.millis) : "O histórico do ESP32 não registra quedas confirmadas.");
}
async function request(path, options = {}) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), TIMEOUT_MS);
  try {
    const res = await fetch(API_BASE + path, { ...options, signal: controller.signal, cache: "no-store" });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return await res.json();
  } finally { clearTimeout(timeout); }
}
async function refresh() {
  if (busy) return;
  busy = true;
  try {
    const status = await request("/api/status");
    if (!status || typeof status.estado !== "string") throw new Error("Resposta de status inválida");
    updateConnection(true);
    updateStatus(status);
    try {
      const result = await request("/api/history");
      const history = Array.isArray(result) ? result : result.eventos;
      if (!Array.isArray(history)) throw new Error("Histórico inválido");
      lastHistory = history;
      showHistory(history);
      updateLastAlert();
    } catch (error) {
      lastHistory = [];
      showHistory([]);
      setPanel("lastAlert", "alert empty", null, "Histórico indisponível", "Verifique a rota /api/history no ESP32.");
    }
  } catch (error) {
    updateConnection(false);
    setText("clock", "Sem conexão");
    showHistory(lastHistory);
    setPanel("lastAlert", "alert empty", null, "Sem comunicação", "Não é possível consultar o último alerta.");
  } finally { busy = false; }
}
document.getElementById("refresh").addEventListener("click", refresh);
document.getElementById("cancel").addEventListener("click", async () => {
  if (!online || !lastStatus || !(lastStatus.alerta === true || String(lastStatus.estado).toUpperCase() === "ALERTA")) return;
  if (!confirm("Enviar solicitação de cancelamento ao ESP32?")) return;
  try {
    await request("/api/cancel", { method: "POST" });
    await refresh();
  } catch (error) {
    alert("Não foi possível cancelar pelo painel. Use o botão físico do dispositivo.");
  }
});
showHistory([]);
refresh();
setInterval(refresh, POLL_MS);
)WFJS";
