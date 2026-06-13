#include "html_pages.h"

// ============================================================================
// LOGIN PAGE
// ============================================================================
const char* LOGIN_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DOZOWNIK - Login</title>
    <style>
:root{--bg-primary:#0a0f1a;--bg-card:#111827;--bg-input:#1e293b;--border:#2d3a4f;--text-primary:#f1f5f9;--text-secondary:#94a3b8;--text-muted:#64748b;--accent-blue:#38bdf8;--accent-cyan:#22d3d5;--accent-green:#22c55e;--accent-red:#ef4444;--radius:12px;--radius-sm:8px}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg-primary);color:var(--text-primary);min-height:100vh;display:flex;justify-content:center;align-items:center}
body::before{content:'';position:fixed;top:0;left:0;right:0;bottom:0;background:radial-gradient(ellipse at 20% 0%,rgba(56,189,248,0.08) 0%,transparent 50%),radial-gradient(ellipse at 80% 100%,rgba(34,211,213,0.06) 0%,transparent 50%);pointer-events:none;z-index:0}
.login-box{background:var(--bg-card);border:1px solid var(--border);padding:40px;border-radius:var(--radius);box-shadow:0 4px 24px rgba(0,0,0,0.4);width:100%;max-width:400px;margin:20px;position:relative;z-index:1}
.logo{display:flex;align-items:center;justify-content:center;gap:12px;margin-bottom:30px}
.logo-icon{width:40px;height:40px;background:linear-gradient(135deg,var(--accent-cyan),var(--accent-blue));border-radius:var(--radius-sm);display:flex;align-items:center;justify-content:center}
.logo-icon svg{width:24px;height:24px;fill:var(--bg-primary)}
h1{font-size:1.25rem;font-weight:700}
h1 span{color:var(--text-muted);font-weight:500}
.form-group{margin-bottom:20px}
label{display:block;margin-bottom:8px;font-weight:500;color:var(--text-secondary);font-size:0.875rem}
input[type="password"]{width:100%;padding:12px 16px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);font-size:1rem;color:var(--text-primary)}
input[type="password"]:focus{outline:none;border-color:var(--accent-blue);box-shadow:0 0 0 3px rgba(56,189,248,0.2)}
.login-btn{width:100%;padding:14px;background:linear-gradient(135deg,var(--accent-cyan),var(--accent-blue));color:var(--bg-primary);border:none;border-radius:var(--radius-sm);font-size:1rem;font-weight:600;cursor:pointer;transition:all 0.2s}
.login-btn:hover{transform:translateY(-2px);box-shadow:0 4px 12px rgba(56,189,248,0.3)}
.alert{padding:12px 16px;margin:15px 0;border-radius:var(--radius-sm);display:none;font-size:0.875rem;background:rgba(239,68,68,0.15);border:1px solid rgba(239,68,68,0.3);color:var(--accent-red)}
.info{margin-top:24px;padding:16px;background:var(--bg-input);border-radius:var(--radius-sm);font-size:0.75rem;color:var(--text-muted)}
.info strong{color:var(--text-secondary)}
    </style>
</head>
<body>
    <div class="login-box">
        <div class="logo">
            <div class="logo-icon">
                <svg viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg>
            </div>
            <h1>DOZOWNIK <span>– System</span></h1>
        </div>
        <form id="loginForm">
            <div class="form-group">
                <label for="password">Administrator Password</label>
                <input type="password" id="password" name="password" required />
            </div>
            <button type="submit" class="login-btn">Login</button>
        </form>
        <div id="error" class="alert"></div>
        <div class="info">
            <strong>Dosing System v1.0</strong><br />
            6-channel automatic fertilizer doser<br />
            Session-based authentication
        </div>
    </div>
    <script>
document.getElementById("loginForm").addEventListener("submit",function(e){
e.preventDefault();
const password=document.getElementById("password").value;
const errorDiv=document.getElementById("error");
fetch("api/login",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"password="+encodeURIComponent(password)})
.then(r=>r.json())
.then(data=>{if(data.success){window.location.href="./";}else{errorDiv.textContent=data.error||"Login failed";errorDiv.style.display="block";}})
.catch(()=>{errorDiv.textContent="Connection error";errorDiv.style.display="block";});
});
    </script>
</body>
</html>
)rawliteral";

// ============================================================================
// DASHBOARD PAGE
// ============================================================================
const char* DASHBOARD_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>DOZOWNIK - Dashboard</title>
    <style>
:root{
--bg-primary:#0a0f1a;--bg-card:#111827;--bg-card-hover:#1a2332;--bg-input:#1e293b;
--border:#2d3a4f;--border-light:#3d4a5f;
--text-primary:#f1f5f9;--text-secondary:#94a3b8;--text-muted:#64748b;
--accent-blue:#38bdf8;--accent-cyan:#22d3d5;--accent-green:#22c55e;--accent-red:#ef4444;--accent-orange:#f97316;--accent-yellow:#eab308;--accent-blue-run:#3b82f6;
--state-inactive:#4b5563;--state-incomplete:#eab308;--state-invalid:#ef4444;--state-configured:#22c55e;--state-pending:#38bdf8;
--radius-sm:6px;--radius-md:10px;--radius-lg:14px;
--transition-fast:0.15s ease;--transition-normal:0.25s ease;--transition-slow:0.4s cubic-bezier(0.25,0.46,0.45,0.94);
--card-width:800px;--card-padding:12px;--section-gap:10px;--section-padding:10px;
--event-slot-height:38px;--day-slot-height:36px;--input-height:38px;--btn-height:40px;--header-height:44px;
--font-xs:0.6rem;--font-sm:0.7rem;--font-md:0.8rem;--font-lg:0.95rem;
}
*{margin:0;padding:0;box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{height:100%;touch-action:pan-y}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg-primary);color:var(--text-primary);line-height:1.4}
body::before{content:'';position:fixed;top:0;left:0;right:0;bottom:0;background:radial-gradient(ellipse at 20% 0%,rgba(56,189,248,0.06) 0%,transparent 50%),radial-gradient(ellipse at 80% 100%,rgba(34,211,213,0.04) 0%,transparent 50%);pointer-events:none;z-index:0}

.app-container{display:flex;flex-direction:column;width:100vw;height:100dvh;overflow:hidden;z-index:1}
.card-topbar{display:flex;align-items:center;justify-content:space-between;padding:0 var(--card-padding);background:var(--bg-card);border-bottom:1px solid var(--border);height:var(--header-height);flex-shrink:0}
.card-time{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-md);font-weight:600;color:var(--text-secondary)}
.channel-nav{display:grid;grid-template-columns:repeat(8,1fr);gap:4px;padding:8px var(--card-padding);background:var(--bg-card);border-bottom:1px solid var(--border);flex-shrink:0}
.ch-nav-btn{height:32px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);color:var(--text-secondary);font-size:var(--font-sm);font-weight:600;cursor:pointer;transition:all var(--transition-fast)}
.ch-nav-btn:hover{border-color:var(--border-light);color:var(--text-primary)}
.ch-nav-btn.configured{background:rgba(34,197,94,0.15)}
.ch-nav-btn.pending{background:rgba(56,189,248,0.12);animation:pulse 1.5s infinite}
.ch-nav-btn.invalid{background:rgba(239,68,68,0.12)}
.ch-nav-btn.incomplete{background:rgba(234,179,8,0.12)}
.ch-nav-btn.active{background:rgba(34,211,213,0.15);border-color:var(--accent-cyan);color:var(--accent-cyan)}
.channels-wrapper{flex:1;min-height:0;overflow:hidden;position:relative}
.channels-screen{width:100%;height:100%;position:relative}

.logo{display:flex;align-items:center;gap:8px}
.logo-icon{width:32px;height:32px;background:linear-gradient(135deg,var(--accent-cyan),var(--accent-blue));border-radius:var(--radius-sm);display:flex;align-items:center;justify-content:center}
.logo-icon svg{width:18px;height:18px;fill:var(--bg-primary)}
.logo-text{margin-left:10px;font-size:1rem;font-weight:700;margin-top:3px}
.header-actions{display:flex;gap:6px}
.btn-icon{width:32px;height:32px;border-radius:var(--radius-sm);border:1px solid var(--border);background:var(--bg-input);color:var(--text-secondary);display:flex;align-items:center;justify-content:center;cursor:pointer;transition:all var(--transition-fast)}
.btn-icon:hover{background:var(--bg-card-hover);color:var(--text-primary)}
.btn-icon svg{width:16px;height:16px}

@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}

.channel-card{background:var(--bg-primary);position:absolute;top:0;left:0;width:100%;height:100%;overflow:hidden;opacity:0;display:flex;justify-content:center;pointer-events:none;transition:opacity 0.3s ease}
.channel-card.active{opacity:1;pointer-events:auto}
.card-inner{width:100%;max-width:var(--card-width);height:100%;display:flex;flex-direction:column;border-left:1px solid var(--border);border-right:1px solid var(--border)}
.card-body{flex:1;overflow-y:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.4) transparent}
.card-body::-webkit-scrollbar{width:8px}
.card-body::-webkit-scrollbar-thumb{background-color:rgba(255,255,255,0.25);border-radius:6px}

.state-badge{padding:3px 8px;border-radius:10px;font-size:var(--font-xs);font-weight:700;text-transform:uppercase;letter-spacing:0.05em}
.state-badge.inactive{background:rgba(75,85,99,0.2);color:var(--state-inactive)}
.state-badge.incomplete{background:rgba(234,179,8,0.15);color:var(--state-incomplete)}
.state-badge.invalid{background:rgba(239,68,68,0.15);color:var(--state-invalid)}
.state-badge.configured{background:rgba(34,197,94,0.15);color:var(--state-configured)}
.state-badge.pending{background:rgba(56,189,248,0.15);color:var(--state-pending)}

.card-content{padding:var(--card-padding);display:flex;flex-direction:column;gap:var(--section-gap)}
.section{background:var(--bg-card);border:1px solid var(--border);border-radius:var(--radius-md);overflow:hidden;flex-shrink:0}
.section-header{display:flex;align-items:center;justify-content:space-between;padding:8px var(--section-padding);background:rgba(0,0,0,0.2);border-bottom:1px solid var(--border)}
.section-title{display:flex;align-items:center;gap:6px;font-size:var(--font-sm);font-weight:600;text-transform:uppercase;letter-spacing:0.05em;color:var(--text-secondary)}
.section-title svg{width:12px;height:12px;color:var(--accent-cyan)}
.section-info{font-size:var(--font-xs);color:var(--text-muted)}
.section-body{padding:var(--section-padding)}
.notes-section{background:var(--bg-card);border:1px solid var(--border);border-radius:var(--radius-md);overflow:hidden;flex-shrink:0}
.notes-header{display:flex;align-items:center;padding:9px var(--section-padding);background:rgba(0,0,0,0.2);cursor:default;gap:8px;min-height:40px;border-bottom:1px solid transparent;transition:border-bottom-color var(--transition-fast)}
.notes-section.expanded .notes-header{border-bottom-color:var(--border)}
.notes-icon{flex-shrink:0;width:12px;height:12px;color:var(--accent-cyan)}
.notes-preview{flex:1;font-size:1rem;font-weight:700;color:var(--text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;letter-spacing:0.01em}
.notes-preview.empty{font-style:italic;font-weight:400;font-size:var(--font-sm);color:var(--text-muted)}
.notes-toggle-btn{flex-shrink:0;width:24px;height:24px;background:var(--bg-input);border:1px solid var(--border);border-radius:6px;color:var(--text-muted);cursor:pointer;display:flex;align-items:center;justify-content:center;transition:all var(--transition-fast);font-size:10px;line-height:1}
.notes-toggle-btn:hover{border-color:var(--accent-cyan);color:var(--accent-cyan);background:rgba(34,211,213,0.08)}
.notes-list{display:none;flex-direction:column;gap:4px;padding:8px var(--section-padding)}
.notes-section.expanded .notes-list{display:flex}
.note-entry{display:flex;align-items:center;gap:6px}
.note-idx{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-xs);color:var(--text-muted);width:18px;text-align:right;flex-shrink:0}
.note-input{flex:1;height:30px;padding:0 8px;background:var(--bg-input);border:1px solid var(--border);border-radius:6px;font-size:var(--font-md);color:var(--accent-cyan);outline:none;transition:border-color var(--transition-fast),box-shadow var(--transition-fast)}
.note-input::placeholder{color:var(--text-muted);font-style:italic;font-size:var(--font-xs)}
.note-input:focus{border-color:var(--accent-cyan);box-shadow:0 0 0 2px rgba(34,211,213,0.12);color:var(--text-primary)}
.note-input.sel{border-color:rgba(34,211,213,0.35);background:rgba(34,211,213,0.03)}
.note-sel-btn{flex-shrink:0;width:18px;height:18px;border:none;background:none;color:var(--text-muted);cursor:pointer;display:flex;align-items:center;justify-content:center;border-radius:4px;transition:color var(--transition-fast);padding:0}
.note-sel-btn:hover,.note-sel-btn.active{color:var(--accent-cyan)}
.note-sel-btn svg{width:10px;height:10px}
.notes-hint{font-size:var(--font-xs);color:var(--text-muted);text-align:center;padding:2px 0;font-style:italic}

.events-grid{display:grid;grid-template-columns:repeat(6,1fr);gap:4px}
.event-slot{position:relative}
.event-cb{display:none}
.event-lbl{display:flex;flex-direction:column;align-items:center;justify-content:center;height:var(--event-slot-height);background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);cursor:pointer;transition:all var(--transition-fast)}
.event-lbl:hover{border-color:var(--accent-cyan);background:var(--bg-card-hover)}
.event-cb:checked+.event-lbl{background:rgba(34,211,213,0.15);border-color:var(--accent-cyan)}
.event-time{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-xs);font-weight:600;color:var(--text-secondary)}
.event-cb:checked+.event-lbl .event-time{color:var(--accent-cyan)}
.event-dot{width:4px;height:4px;border-radius:50%;margin-top:3px;background:transparent}
.event-cb:checked+.event-lbl .event-dot{background:var(--accent-cyan)}
.event-slot.done .event-lbl{background:rgba(34,197,94,0.15);border-color:var(--accent-green)}
.event-slot.done .event-time{color:var(--accent-green)}
.event-slot.done .event-dot{background:var(--accent-green)}
.event-slot.failed .event-lbl{background:rgba(239,68,68,0.15);border-color:#ef4444}
.event-slot.failed .event-time{color:#ef4444}
.event-slot.failed .event-dot{background:#ef4444}
.event-slot.next .event-lbl{background:rgba(234,179,8,0.15);border-color:var(--accent-yellow);animation:pulse-border 2s infinite}
.event-slot.next .event-time{color:var(--accent-yellow)}
.event-slot.running .event-lbl{background:rgba(59,130,246,0.2);border-color:var(--accent-blue-run);animation:running-pulse 1s infinite}
.event-slot.running .event-time{color:var(--accent-blue-run)}
.event-slot.running .event-dot{background:var(--accent-blue-run)}

@keyframes running-pulse{0%,100%{box-shadow:0 0 0 0 rgba(59,130,246,0.5)}50%{box-shadow:0 0 8px 2px rgba(59,130,246,0.3)}}
@keyframes pulse-border{0%,100%{box-shadow:0 0 0 0 rgba(234,179,8,0.4)}50%{box-shadow:0 0 0 2px rgba(234,179,8,0.1)}}

.days-grid{display:grid;grid-template-columns:repeat(7,1fr);gap:4px}
.day-slot{position:relative}
.day-cb{display:none}
.day-lbl{display:flex;align-items:center;justify-content:center;height:var(--day-slot-height);background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);cursor:pointer;transition:all var(--transition-fast)}
.day-lbl:hover{border-color:var(--accent-cyan);background:var(--bg-card-hover)}
.day-cb:checked+.day-lbl{background:rgba(34,211,213,0.15);border-color:var(--accent-cyan)}
.day-name{font-size:var(--font-sm);font-weight:700;text-transform:uppercase;letter-spacing:0.03em;color:var(--text-secondary)}
.day-cb:checked+.day-lbl .day-name{color:var(--accent-cyan)}
.day-slot.today .day-lbl{border-width:2px}
.day-slot.today .day-name::after{content:'';display:inline-block;width:4px;height:4px;background:var(--accent-yellow);border-radius:50%;margin-left:3px;vertical-align:middle}

.volume-input{width:100%;height:var(--input-height);padding:0 10px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);color:var(--accent-cyan);text-align:center}
.volume-input:focus{outline:none;border-color:var(--accent-cyan);box-shadow:0 0 0 2px rgba(34,211,213,0.15)}
.params-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-bottom:0}
.param-item{display:flex;flex-direction:column;gap:4px}
.param-lbl{font-size:var(--font-xs);font-weight:600;text-transform:uppercase;letter-spacing:0.05em;color:var(--text-muted)}
.param-lbl.low{color:var(--accent-red)}
.param-val{height:var(--day-slot-height);background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);font-weight:700;letter-spacing:0.03em;color:var(--accent-cyan);display:flex;align-items:center;justify-content:center}
.param-bar-box{min-height:var(--input-height);background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);padding:5px 8px;display:flex;flex-direction:column;justify-content:center;gap:4px}
.param-bar-val{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);font-weight:600;color:var(--accent-cyan)}
.param-bar-val.low{color:var(--accent-red)}
.valid-msg{display:flex;align-items:center;gap:6px;padding:8px 10px;border-radius:var(--radius-sm);font-size:var(--font-sm);font-weight:500;flex-shrink:0}
.valid-msg svg{width:14px;height:14px;flex-shrink:0}
.valid-msg.err{background:rgba(239,68,68,0.1);border:1px solid rgba(239,68,68,0.3);color:var(--accent-red)}
.valid-msg.warn{background:rgba(234,179,8,0.1);border:1px solid rgba(234,179,8,0.3);color:var(--accent-yellow)}
.valid-msg.info{background:rgba(56,189,248,0.1);border:1px solid rgba(56,189,248,0.3);color:var(--accent-blue)}
.valid-msg.ok{background:rgba(34,197,94,0.1);border:1px solid rgba(34,197,94,0.3);color:var(--accent-green);margin-top:8px}

.btn{flex:1;height:var(--btn-height);border-radius:var(--radius-sm);font-size:var(--font-sm);font-weight:600;cursor:pointer;transition:all var(--transition-fast);display:flex;align-items:center;justify-content:center;gap:5px}
.btn svg{width:14px;height:14px}
.btn-primary{background:linear-gradient(135deg,var(--accent-cyan),var(--accent-blue));border:none;color:var(--bg-primary)}
.btn-primary:hover{transform:translateY(-1px);box-shadow:0 4px 12px rgba(34,211,213,0.3)}
.btn-primary:disabled{opacity:0.5;cursor:not-allowed;transform:none;box-shadow:none}
.btn-secondary{background:var(--bg-input);border:1px solid var(--border);color:var(--text-secondary)}
.btn-secondary:hover{background:var(--bg-card-hover);color:var(--text-primary)}


@media (max-width:768px) and (hover:none){
.channel-nav{grid-template-columns:repeat(4,1fr)}
.card-inner{border-left:none;border-right:none}
.config-groups{flex-direction:column}
.params-grid{grid-template-columns:1fr}
}

.container-bar{width:100%;height:8px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);overflow:hidden}
.container-bar-fill{height:100%;background:linear-gradient(90deg,var(--accent-cyan),var(--accent-blue));transition:width 0.3s ease;border-radius:var(--radius-sm) 0 0 var(--radius-sm)}
.container-bar-fill.low{background:linear-gradient(90deg,var(--accent-red),var(--accent-orange))}
.container-bar-fill.dosed{background:linear-gradient(90deg,var(--accent-blue),var(--accent-cyan))}
.params-actions{display:flex;gap:8px;margin-top:4px}
.params-actions .btn{flex:1}
.config-groups{display:flex;gap:8px;margin-bottom:8px}
.config-group{flex:1;padding:8px;border-radius:var(--radius-sm);background:rgba(255,255,255,0.02);border:1px solid rgba(255,255,255,0.05)}
.dose-group{border-top:2px solid rgba(34,211,213,0.25)}
.container-group{border-top:2px solid rgba(56,189,248,0.25)}
.param-lbl-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:2px}
.param-unit{font-size:var(--font-xs);font-weight:600;text-transform:uppercase;letter-spacing:0.05em;color:var(--text-muted)}
.calib-section{margin-bottom:8px}
.calib-inner{display:flex;height:var(--btn-height);border:1px solid var(--border);border-radius:var(--radius-sm);overflow:hidden}
.calib-input-field{flex:1;background:var(--bg-input);border:none;padding:0 12px;font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-md);font-weight:600;color:var(--text-secondary);text-align:center;min-width:0}
.calib-input-field:focus{outline:none;box-shadow:inset 0 0 0 1px rgba(34,211,213,0.3)}
.calib-input-field::placeholder{color:var(--text-muted);font-size:var(--font-sm);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}
.calib-run-btn{background:rgba(34,211,213,0.1);border:none;border-left:1px solid var(--border);padding:0 16px;color:var(--accent-cyan);font-size:var(--font-sm);font-weight:600;cursor:pointer;transition:all var(--transition-fast);white-space:nowrap;flex-shrink:0}
.calib-run-btn:hover{background:rgba(34,211,213,0.2)}
.calib-run-btn.running{background:rgba(234,179,8,0.15);border-left-color:rgba(234,179,8,0.3);color:var(--accent-yellow);pointer-events:none}

/* Modal */
.modal-overlay{position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.7);backdrop-filter:blur(4px);display:flex;align-items:center;justify-content:center;z-index:1000;opacity:0;visibility:hidden;transition:all 0.2s ease}
.modal-overlay.show{opacity:1;visibility:visible}
.modal-box{background:var(--bg-card);border:1px solid var(--border);border-radius:var(--radius-lg);padding:24px;max-width:360px;width:90%;transform:scale(0.9);transition:transform 0.2s ease}
.modal-overlay.show .modal-box{transform:scale(1)}
.modal-icon{width:48px;height:48px;margin:0 auto 16px;background:rgba(34,197,94,0.15);border-radius:50%;display:flex;align-items:center;justify-content:center}
.modal-icon svg{width:24px;height:24px;color:var(--accent-green)}
.modal-title{font-size:var(--font-lg);font-weight:700;text-align:center;margin-bottom:8px;color:var(--text-primary)}
.modal-text{font-size:var(--font-sm);text-align:center;color:var(--text-secondary);margin-bottom:20px;line-height:1.5}
.modal-info{background:var(--bg-input);border-radius:var(--radius-sm);padding:12px;margin-bottom:20px;text-align:center}
.modal-info-label{font-size:var(--font-xs);color:var(--text-muted);text-transform:uppercase;letter-spacing:0.05em}
.modal-info-value{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-lg);font-weight:700;color:var(--accent-cyan)}
.modal-actions{display:flex;gap:10px}
.modal-actions .btn{flex:1}

.modal-icon.ok{background:rgba(34,197,94,0.15)}.modal-icon.ok svg{color:var(--accent-green)}
.modal-icon.err{background:rgba(239,68,68,0.15)}.modal-icon.err svg{color:var(--accent-red)}
.modal-icon.warn{background:rgba(234,179,8,0.15)}.modal-icon.warn svg{color:var(--accent-yellow)}
.modal-icon.info{background:rgba(56,189,248,0.15)}.modal-icon.info svg{color:var(--accent-blue)}
.params-section{background:var(--bg-card);border:1px solid var(--border);border-radius:var(--radius-md);overflow:hidden;flex-shrink:0}
.params-header{display:flex;align-items:center;padding:9px var(--section-padding);background:rgba(0,0,0,0.2);cursor:default;gap:8px;min-height:40px;border-bottom:1px solid transparent;transition:border-bottom-color var(--transition-fast)}
.params-section.expanded .params-header{border-bottom-color:var(--border)}
.params-icon{flex-shrink:0;width:12px;height:12px;color:var(--accent-cyan)}
.params-preview{flex:1;font-size:var(--font-sm);font-weight:600;color:var(--text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.params-preview.empty{font-style:italic;font-weight:400;color:var(--text-muted)}
.params-toggle-btn{flex-shrink:0;width:24px;height:24px;background:var(--bg-input);border:1px solid var(--border);border-radius:6px;color:var(--text-muted);cursor:pointer;display:flex;align-items:center;justify-content:center;transition:all var(--transition-fast);font-size:10px;line-height:1}
.params-toggle-btn:hover{border-color:var(--accent-cyan);color:var(--accent-cyan);background:rgba(34,211,213,0.08)}
.params-body{display:none;flex-direction:column;gap:8px;padding:8px var(--section-padding)}
.params-section.expanded .params-body{display:flex}
.tmpl-block{background:rgba(0,0,0,0.18);border:1px solid var(--border);border-radius:var(--radius-sm);overflow:hidden}
.tmpl-header{display:flex;align-items:center;gap:5px;padding:5px 8px;background:rgba(255,255,255,0.025);border-bottom:1px solid var(--border)}
.tmpl-dot{width:6px;height:6px;border-radius:50%;background:var(--accent-cyan);flex-shrink:0}
.tmpl-name{flex:1;font-size:var(--font-sm);font-weight:600;color:var(--text-primary)}
.tmpl-unit{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-xs);color:var(--text-muted);background:var(--bg-input);padding:1px 5px;border-radius:3px;flex-shrink:0}
.tmpl-icon-btn{height:22px;width:22px;padding:3px;background:none;border:1px solid var(--border);border-radius:4px;color:var(--text-muted);cursor:pointer;flex-shrink:0;display:flex;align-items:center;justify-content:center;transition:all var(--transition-fast)}
.tmpl-icon-btn svg{width:100%;height:100%}
.tmpl-icon-btn:hover{border-color:var(--accent-cyan);color:var(--accent-cyan);background:rgba(34,211,213,0.08)}
.tmpl-icon-btn.active{border-color:var(--accent-cyan);color:var(--accent-cyan);background:rgba(34,211,213,0.12)}
.tmpl-icon-btn.csv{cursor:not-allowed;opacity:0.4}
.tmpl-icon-btn.csv:hover{border-color:var(--border);color:var(--text-muted);background:none}
.tmpl-add-btn{height:22px;padding:0 8px;background:rgba(34,211,213,0.08);border:1px solid rgba(34,211,213,0.25);border-radius:4px;color:var(--accent-cyan);font-size:var(--font-xs);font-weight:600;cursor:pointer;transition:all var(--transition-fast);flex-shrink:0}
.tmpl-add-btn:hover{background:rgba(34,211,213,0.18)}
.tmpl-remove-btn{height:22px;width:22px;background:none;border:1px solid var(--border);border-radius:4px;color:var(--text-muted);font-size:12px;cursor:pointer;flex-shrink:0;display:flex;align-items:center;justify-content:center;transition:all var(--transition-fast)}
.tmpl-remove-btn:hover{border-color:var(--accent-red);color:var(--accent-red);background:rgba(239,68,68,0.06)}
.chart-container{border-bottom:1px solid var(--border);background:rgba(0,0,0,0.25)}
.chart-wrap{display:flex;align-items:stretch}
.chart-scroll-btn{flex-shrink:0;width:26px;background:none;border:none;color:var(--text-muted);cursor:pointer;font-size:12px;display:flex;align-items:center;justify-content:center;transition:color var(--transition-fast)}
.chart-scroll-btn:hover:not(:disabled){color:var(--accent-cyan)}
.chart-scroll-btn:disabled{opacity:0.2;cursor:default}
.chart-canvas{flex:1;height:140px;display:block;min-width:0}
.chart-footer{padding:3px 8px 5px;text-align:center;font-size:var(--font-xs);color:var(--text-muted);font-style:italic}
.tmpl-records{max-height:196px;overflow-y:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.15) transparent}
.add-record-form{display:none;align-items:center;gap:6px;padding:5px 8px;background:rgba(34,211,213,0.04);border-bottom:1px solid rgba(34,211,213,0.12)}
.add-record-form.visible{display:flex}
.add-rec-input{width:88px;height:26px;padding:0 6px;background:var(--bg-input);border:1px solid var(--accent-cyan);border-radius:4px;font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);color:var(--accent-cyan);outline:none}
.add-rec-unit{font-size:var(--font-xs);color:var(--text-muted);flex-shrink:0;min-width:28px}
.add-rec-save{height:26px;padding:0 10px;background:var(--accent-cyan);border:none;border-radius:4px;color:var(--bg-primary);font-size:var(--font-xs);font-weight:700;cursor:pointer}
.add-rec-cancel{height:26px;width:26px;background:none;border:1px solid var(--border);border-radius:4px;color:var(--text-muted);font-size:13px;cursor:pointer;display:flex;align-items:center;justify-content:center}
.record-row{display:flex;align-items:center;gap:8px;padding:0 8px;height:32px;border-bottom:1px solid rgba(255,255,255,0.03);transition:background var(--transition-fast)}
.record-row:last-child{border-bottom:none}
.record-row:hover{background:rgba(255,255,255,0.02)}
.rec-value{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);font-weight:600;color:var(--accent-cyan);flex:0 0 80px}
.rec-value-input{width:76px;height:22px;padding:0 4px;background:var(--bg-input);border:1px solid var(--accent-cyan);border-radius:3px;font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);color:var(--accent-cyan);outline:none;flex:0 0 76px}
.rec-ts{flex:1;font-size:var(--font-xs);color:var(--text-muted)}
.rec-actions{flex-shrink:0;display:flex;gap:2px}
.rec-btn{width:20px;height:20px;border:none;background:none;color:var(--text-muted);font-size:11px;cursor:pointer;border-radius:3px;display:flex;align-items:center;justify-content:center;transition:all var(--transition-fast)}
.rec-btn:hover{background:var(--bg-input);color:var(--text-primary)}
.rec-btn.del:hover{color:var(--accent-red)}
.records-empty{padding:10px 8px;text-align:center;font-size:var(--font-xs);color:var(--text-muted);font-style:italic}
.params-bottom{display:flex;flex-direction:column;gap:5px}
.assign-row{display:flex;gap:6px;align-items:center}
.assign-select{flex:1;height:30px;padding:0 8px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);color:var(--text-secondary);font-size:var(--font-sm);outline:none;appearance:none;cursor:pointer}
.assign-select:focus{border-color:var(--accent-cyan)}
.assign-btn{height:30px;padding:0 12px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);color:var(--text-secondary);font-size:var(--font-sm);cursor:pointer;transition:all var(--transition-fast);flex-shrink:0}
.assign-btn:hover{border-color:var(--accent-cyan);color:var(--accent-cyan)}
.new-tmpl-btn{width:100%;height:28px;background:none;border:1px dashed var(--border);border-radius:var(--radius-sm);color:var(--text-muted);font-size:var(--font-xs);cursor:pointer;transition:all var(--transition-fast)}
.new-tmpl-btn:hover{border-color:var(--accent-cyan);color:var(--accent-cyan)}
.new-tmpl-form{display:none;align-items:center;gap:6px}
.new-tmpl-form.visible{display:flex}
.new-tmpl-name{flex:1;height:30px;padding:0 8px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);color:var(--accent-cyan);font-size:var(--font-sm);outline:none}
.new-tmpl-name:focus{border-color:var(--accent-cyan)}
.new-tmpl-unit{width:62px;height:30px;padding:0 8px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);color:var(--accent-cyan);font-size:var(--font-sm);outline:none;flex-shrink:0}
.new-tmpl-unit:focus{border-color:var(--accent-cyan)}
.new-tmpl-create{height:30px;padding:0 10px;background:var(--accent-cyan);border:none;border-radius:var(--radius-sm);color:var(--bg-primary);font-size:var(--font-xs);font-weight:700;cursor:pointer;flex-shrink:0}
.new-tmpl-cancel{height:30px;width:30px;background:none;border:1px solid var(--border);border-radius:var(--radius-sm);color:var(--text-muted);font-size:13px;cursor:pointer;flex-shrink:0;display:flex;align-items:center;justify-content:center}
.params-empty{padding:18px 8px;text-align:center;font-size:var(--font-sm);color:var(--text-muted);font-style:italic}
</style>
</head>
<body>
    <div class="app-container">
        <div class="channels-wrapper">
            <div class="channels-screen" id="channelScreen"></div>
        </div>
        <div class="modal-overlay" id="refillModal">
            <div class="modal-box">
                <div class="modal-icon"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg></div>
                <div class="modal-title">Confirm Refill</div>
                <div class="modal-text">Reset remaining volume to full container capacity?</div>
                <div class="modal-info"><div class="modal-info-label">Channel <span id="modalChannel">1</span></div><div class="modal-info-value"><span id="modalCapacity">1000</span> ml</div></div>
                <div class="modal-actions"><button class="btn btn-secondary" onclick="closeRefillModal()">Cancel</button><button class="btn btn-primary" onclick="confirmRefill()">Refill</button></div>
            </div>
        </div>
        <div class="modal-overlay" id="alertModal"><div class="modal-box"><div class="modal-icon" id="alertIcon"></div><div class="modal-title" id="alertTitle"></div><div class="modal-text" id="alertText"></div><div class="modal-actions" id="alertActions"></div></div></div>
        <div class="modal-overlay" id="saveModal">
            <div class="modal-box">
                <div class="modal-icon info"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 01-2-2V5a2 2 0 012-2h11l5 5v11a2 2 0 01-2 2z"/><polyline points="17,21 17,13 7,13 7,21"/><polyline points="7,3 7,8 15,8"/></svg></div>
                <div class="modal-title">Save changes?</div>
                <div class="modal-text">Configuration will be active from tomorrow.</div>
                <div class="modal-info"><div class="modal-info-label">Channel <span id="saveModalChannel">1</span></div></div>
                <div class="modal-actions"><button class="btn btn-secondary" onclick="closeSaveModal()">Cancel</button><button class="btn btn-primary" onclick="confirmSave()">Save</button></div>
            </div>
        </div>
        <div class="modal-overlay" id="sessionExpiredModal">
            <div class="modal-box">
                <div class="modal-icon warn"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg></div>
                <div class="modal-title">Session Expired</div>
                <div class="modal-text">Your session has expired. Please log in again.</div>
                <div class="modal-actions"><a class="btn btn-primary" href="login" onclick="if(window.location.pathname.indexOf('/device/')!==-1){window.location.href='/dashboard';return false;}">Login</a></div>
            </div>
        </div>
    </div>
<script>
const CFG={CHANNEL_COUNT:8,EVENTS_PER_DAY:11,FIRST_EVENT_HOUR:2,LAST_EVENT_HOUR:22,CHANNEL_OFFSET_MIN:15,EVENT_WINDOW_SEC:300,MAX_PUMP_SEC:180,MIN_DOSE_ML:0.1,MIN_RATE:1/30,MAX_RATE:5.0,CALIB_SEC:30,SWIPE_THRESHOLD:50};
const DAYS=['Mon','Tue','Wed','Thu','Fri','Sat','Sun'];

let channels=[];
let currentCh=0;
let activeChannel=-1;
let activeEventHour=-1;
let touchStart={x:0,y:0};
let editingChannel=-1;
let pendingRefillChannel=-1;
let sharedNotes=Array(12).fill('');
let chNoteIdx=Array(8).fill(0);
let notesExpanded=Array(8).fill(false);
let editingNotes=false;
let paramLog=null;
const paramsExpanded=Array(8).fill(false);
const chartState={};
const chTmplAssign=Array.from({length:8},()=>[]);

function init(){
    for(let i=0;i<CFG.CHANNEL_COUNT;i++){
        channels.push({id:i,events:0,days:0,dailyDose:0,dosingRate:0.33,enabled:true,name:'CH'+i,eventsCompleted:0,eventsFailed:0,state:'inactive',containerMl:1000,remainingMl:1000,remainingPct:100,lowVolume:false,daysRemaining:999,totalDosedMl:0});
    }
    renderChannels();
    goToChannel(0);
    setupTouch();
    updateClock();
    setInterval(updateClock,1000);
    loadStatus();
    setInterval(loadStatus,5000);
    loadNotes();
    loadParamLog();
}


function renderChannels(){
    const slider=document.getElementById('channelScreen');
    const activeCardBody=document.querySelector(`.channel-card[data-ch="${currentCh}"] .card-body`);
    const scrollTop=activeCardBody?activeCardBody.scrollTop:0;
    slider.innerHTML=channels.map((ch,idx)=>renderChannelCard(ch,idx)).join('');
    channels.forEach((ch,idx)=>{attachCardEvents(idx);});
    const newActiveCard=document.querySelector(`.channel-card[data-ch="${currentCh}"]`);
    if(newActiveCard){
        newActiveCard.classList.add('active');
        const newCardBody=newActiveCard.querySelector('.card-body');
        if(newCardBody)newCardBody.scrollTop=scrollTop;
    }
    updateClock();
}

function getNextEventHour(ch,chIdx){
    const now=new Date();
    const utcHour=now.getUTCHours();
    const utcMinute=now.getUTCMinutes();
    for(let h=CFG.FIRST_EVENT_HOUR;h<=CFG.LAST_EVENT_HOUR;h+=2){
        if(!(ch.events&(1<<h)))continue;
        if(ch.eventsCompleted&(1<<h))continue;
        const actualH=h+Math.floor(chIdx*CFG.CHANNEL_OFFSET_MIN/60);
        const actualM=(chIdx*CFG.CHANNEL_OFFSET_MIN)%60;
        if(actualH>utcHour)return h;
        if(actualH===utcHour&&actualM>utcMinute)return h;
    }
    return -1;
}

function renderChannelCard(ch,idx){
    const now=new Date();
    const evCnt=popcount(ch.events);
    const dayCnt=popcount(ch.days);
    const single=evCnt>0?ch.dailyDose/evCnt:0;
    const pumpTime=ch.dosingRate>0?single/ch.dosingRate:0;
    const weekly=ch.dailyDose*dayCnt;
    const nextEvent=getNextEventHour(ch,idx);
    const todayIdx=(now.getDay()+6)%7;
    
    let validClass='ok',validMsg='Configuration valid',validIcon='<path d="M22 11.08V12a10 10 0 11-5.93-9.14"/><polyline points="22,4 12,14.01 9,11.01"/>';
    if(evCnt===0){validClass='info';validMsg='Select time slots to activate';validIcon='<circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/>';}
    else if(dayCnt===0){validClass='warn';validMsg='Select active days';validIcon='<path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>';}
    else if(ch.dailyDose<=0){validClass='warn';validMsg='Enter daily dose';validIcon='<path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>';}
    else if(single<CFG.MIN_DOSE_ML){validClass='err';validMsg=`Single dose ${single.toFixed(1)}ml < min ${CFG.MIN_DOSE_ML}ml`;validIcon='<circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/>';}
    else if(pumpTime>CFG.MAX_PUMP_SEC){validClass='err';validMsg=`Pump time ${pumpTime.toFixed(0)}s > max ${CFG.MAX_PUMP_SEC}s`;validIcon='<circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/>';}

    let eventsHtml='';
    // Parzyste godziny 02,04,...,22; rzeczywisty czas wykonania uwzględnia offset kanału
    const chOffsetMin=idx*CFG.CHANNEL_OFFSET_MIN;
    const chActualH=Math.floor(chOffsetMin/60);
    const chActualM=chOffsetMin%60;
    for(let h=CFG.FIRST_EVENT_HOUR;h<=CFG.LAST_EVENT_HOUR;h+=2){
        const checked=(ch.events&(1<<h))?'checked':'';
        const done=(ch.eventsCompleted&(1<<h))?'done':'';
        const failed=(ch.eventsFailed&(1<<h))?'failed':'';
        const running=(idx===activeChannel&&h===activeEventHour)?'running':'';
        const next=(h===nextEvent&&!running)?'next':'';
        // Wyświetl rzeczywistą godzinę wykonania (eventHour + offset)
        const dispH=(h+chActualH)%24;
        const timeStr=String(dispH).padStart(2,'0')+':'+String(chActualM).padStart(2,'0');
        eventsHtml+=`<div class="event-slot ${done} ${failed} ${running} ${next}"><input type="checkbox" id="ev_${idx}_${h}" class="event-cb" data-ch="${idx}" data-hour="${h}" ${checked}><label for="ev_${idx}_${h}" class="event-lbl"><span class="event-time">${timeStr}</span><span class="event-dot"></span></label></div>`;
    }
    
    let daysHtml='';
    DAYS.forEach((name,d)=>{
        const checked=(ch.days&(1<<d))?'checked':'';
        const today=(d===todayIdx)?'today':'';
        daysHtml+=`<div class="day-slot ${today}"><input type="checkbox" id="day_${idx}_${d}" class="day-cb" data-ch="${idx}" data-day="${d}" ${checked}><label for="day_${idx}_${d}" class="day-lbl"><span class="day-name">${name}</span></label></div>`;
    });

    const enabledClass=ch.enabled?'':'channel-disabled';
    const navBtns=channels.map((c,i)=>`<button class="ch-nav-btn ${c.state}${i===idx?' active':''}" data-ch="${i}" onclick="goToChannel(${i})">CH ${i+1}</button>`).join('');
    return `<div class="channel-card ${enabledClass}" data-ch="${idx}">
<div class="card-inner">
<div class="card-topbar"><div class="logo"><div class="logo-icon"><svg viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg></div><div class="logo-text">DOZOWNIK</div><span class="state-badge ${ch.state}">${getStateLabel(ch.state)}</span></div><span class="card-time">--:--:--</span><div class="header-actions"><button class="btn-icon" title="Logout" onclick="logout()"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M9 21H5a2 2 0 01-2-2V5a2 2 0 012-2h4"/><polyline points="16,17 21,12 16,7"/><line x1="21" y1="12" x2="9" y2="12"/></svg></button></div></div>
<div class="channel-nav">${navBtns}</div>
<div class="card-body">
<div class="card-content">
${renderNotesSection(idx)}
${renderParamsSection(idx)}
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><polyline points="12,6 12,12 16,14"/></svg>Time Schedule (UTC)</div><div class="section-info" id="evInfo_${idx}">${evCnt} of ${CFG.EVENTS_PER_DAY}</div></div><div class="section-body"><div class="events-grid">${eventsHtml}</div></div></div>
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><rect x="3" y="4" width="18" height="18" rx="2" ry="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/></svg>Active Days</div><div class="section-info" id="dayInfo_${idx}">${dayCnt} of 7</div></div><div class="section-body"><div class="days-grid">${daysHtml}</div></div></div>
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>Configuration</div></div><div class="section-body"><div class="config-groups"><div class="config-group dose-group"><div class="params-grid"><div class="param-item"><label class="param-lbl">Daily Dose (ml)</label><input type="number" class="volume-input" id="dose_${idx}" value="${ch.dailyDose}" step="0.1" min="0" data-ch="${idx}"></div><div class="param-item"><div class="param-lbl">Single Dose</div><div class="param-val" id="single_${idx}">${single.toFixed(1)} ml</div></div><div class="param-item"><div class="param-lbl">Pump Time</div><div class="param-val" id="pumpTime_${idx}">${pumpTime.toFixed(1)} s</div></div><div class="param-item"><div class="param-lbl">Weekly</div><div class="param-val" id="weekly_${idx}">${weekly.toFixed(1)} ml</div></div></div></div><div class="config-group container-group"><div class="params-grid"><div class="param-item"><label class="param-lbl">Container Size (ml)</label><input type="number" class="volume-input" id="container_${idx}" value="${ch.containerMl||1000}" step="10" min="100" max="5000" onchange="saveContainerSize(${idx})"></div><div class="param-item"><div class="param-lbl">Days Left</div><div class="param-val" id="daysLeft_${idx}">${ch.daysRemaining?ch.daysRemaining.toFixed(1):'∞'}</div></div><div class="param-item"><div class="param-lbl-row"><span class="param-lbl ${ch.lowVolume?'low':''}">Remaining</span><span class="param-unit">ML</span></div><div class="param-bar-box"><span class="param-bar-val ${ch.lowVolume?'low':''}" id="remainingLabel_${idx}">${(ch.remainingMl||1000).toFixed(0)}</span><div class="container-bar"><div class="container-bar-fill ${ch.lowVolume?'low':''}" id="containerBar_${idx}" style="width:${ch.remainingPct||100}%"></div></div></div></div><div class="param-item"><div class="param-lbl-row"><span class="param-lbl">Dosed</span><span class="param-unit">ML</span></div><div class="param-bar-box"><span class="param-bar-val" id="dosedLabel_${idx}">${(ch.totalDosedMl||0).toFixed(1)}</span><div class="container-bar"><div class="container-bar-fill dosed" id="dosedBar_${idx}" style="width:${weekly>0?Math.min(100,(ch.totalDosedMl||0)/weekly*100):0}%"></div></div></div></div></div></div></div><div class="calib-section"><div class="param-lbl">Pump Calibration</div><div class="calib-inner"><input type="number" class="calib-input-field" id="calibMl_${idx}" placeholder="— ml" step="0.1" min="0" value="${ch.dosingRate>0?(ch.dosingRate*CFG.CALIB_SEC).toFixed(2):''}" data-ch="${idx}"><button class="calib-run-btn" id="calibBtn_${idx}" onclick="runCalib(${idx})">Run Pump (30s)</button></div></div><div class="params-actions"><button class="btn btn-primary" id="saveBtn_${idx}" onclick="showSaveModal(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 01-2-2V5a2 2 0 012-2h11l5 5v11a2 2 0 01-2 2z"/><polyline points="17,21 17,13 7,13 7,21"/><polyline points="7,3 7,8 15,8"/></svg>Save</button><button class="btn btn-primary" onclick="showRefillModal(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>Refill</button><button class="btn btn-primary" onclick="resetDosed(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M3 12a9 9 0 109-9 9.75 9.75 0 00-6.74 2.74L3 8"/><path d="M3 3v5h5"/></svg>Reset Dosed</button></div><div class="valid-msg ${validClass}" id="validMsg_${idx}"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24">${validIcon}</svg><span id="validTxt_${idx}">${validMsg}</span></div></div></div>
</div>
</div>
</div></div>`;
}

function attachCardEvents(idx){
    const markEditing=()=>{editingChannel=idx;};
    document.querySelectorAll(`.event-cb[data-ch="${idx}"]`).forEach(cb=>{cb.addEventListener('change',()=>{markEditing();updateChannel(idx);});});
    document.querySelectorAll(`.day-cb[data-ch="${idx}"]`).forEach(cb=>{cb.addEventListener('change',()=>{markEditing();updateChannel(idx);});});
    const doseInput=document.getElementById(`dose_${idx}`);
    if(doseInput){doseInput.addEventListener('focus',markEditing);doseInput.addEventListener('input',()=>updateChannel(idx));}
    const calibInput=document.getElementById(`calibMl_${idx}`);
    if(calibInput){calibInput.addEventListener('focus',markEditing);calibInput.addEventListener('change',()=>calcCalibration(idx));}
    const containerInput=document.getElementById(`container_${idx}`);
    if(containerInput){
        containerInput.addEventListener('focus',markEditing);
        containerInput.addEventListener('blur',()=>{
            if(document.activeElement!==doseInput&&document.activeElement!==calibInput)editingChannel=-1;
        });
    }
    for(let ni=0;ni<12;ni++){const inp=document.getElementById('noteInput_'+idx+'_'+ni);if(inp){inp.addEventListener('focus',()=>{editingNotes=true;if(chNoteIdx[idx]!==ni)selectNote(idx,ni);});inp.addEventListener('blur',()=>onNoteBlur(idx,ni,inp.value));inp.addEventListener('keydown',e=>{if(e.key==='Enter'){inp.blur();e.preventDefault();}if(e.key==='Escape'){inp.value=sharedNotes[ni]||'';editingNotes=false;inp.blur();}});}}
}

function loadNotes(){fetch('api/notes').then(r=>r.json()).then(d=>{if(Array.isArray(d.notes))sharedNotes=d.notes;if(Array.isArray(d.ch_note_idx))chNoteIdx=d.ch_note_idx;for(let i=0;i<CFG.CHANNEL_COUNT;i++)refreshNotesPreview(i);}).catch(()=>{});}
function saveNotes(){fetch('api/notes',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({notes:sharedNotes,ch_note_idx:chNoteIdx})}).catch(()=>{});}
function toggleNotes(idx){notesExpanded[idx]=!notesExpanded[idx];const s=document.getElementById('notesSec_'+idx);const b=document.getElementById('notesToggleBtn_'+idx);if(s)s.classList.toggle('expanded',notesExpanded[idx]);if(b)b.textContent=notesExpanded[idx]?'▴':'▾';}
function selectNote(ch,ni){const prev=chNoteIdx[ch];chNoteIdx[ch]=ni;[prev,ni].forEach((i,j)=>{const sel=(j===1);const inp=document.getElementById('noteInput_'+ch+'_'+i);const btn=document.getElementById('noteSelBtn_'+ch+'_'+i);if(inp)inp.classList.toggle('sel',sel);if(btn){btn.classList.toggle('active',sel);btn.innerHTML=selSvg(sel);}});refreshNotesPreview(ch);saveNotes();}
function onNoteBlur(ch,ni,val){editingNotes=false;const t=val.slice(0,30);if(sharedNotes[ni]===t)return;sharedNotes[ni]=t;for(let c=0;c<CFG.CHANNEL_COUNT;c++){const inp=document.getElementById('noteInput_'+c+'_'+ni);if(inp&&inp!==document.activeElement)inp.value=t;refreshNotesPreview(c);}saveNotes();}
function refreshNotesPreview(ch){const p=document.getElementById('notesPreview_'+ch);if(!p)return;const t=sharedNotes[chNoteIdx[ch]]||'';const empty=!t.trim();p.textContent=empty?'— no note —':t;p.classList.toggle('empty',empty);}
function selSvg(f){return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="4"'+(f?' fill="currentColor"':'')+'/></svg>';}
function escAttr(s){return s.replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;');}
function renderNotesSection(idx){const si=chNoteIdx[idx];const st=sharedNotes[si]||'';const empty=!st.trim();const exp=notesExpanded[idx];let entries='';for(let i=0;i<12;i++){const isSel=(i===si);entries+='<div class="note-entry"><span class="note-idx">'+(i+1)+'</span><input class="note-input'+(isSel?' sel':'')+'" id="noteInput_'+idx+'_'+i+'" type="text" maxlength="30" value="'+escAttr(sharedNotes[i]||'')+'" placeholder="— empty —" data-ch="'+idx+'" data-ni="'+i+'"><button class="note-sel-btn'+(isSel?' active':'')+'" id="noteSelBtn_'+idx+'_'+i+'" onclick="selectNote('+idx+','+i+')">'+selSvg(isSel)+'</button></div>';}return '<div class="notes-section'+(exp?' expanded':'')+'" id="notesSec_'+idx+'"><div class="notes-header"><svg class="notes-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8z"/><polyline points="14,2 14,8 20,8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg><span class="notes-preview'+(empty?' empty':'')+'" id="notesPreview_'+idx+'">'+(empty?'— no note —':escAttr(st))+'</span><button class="notes-toggle-btn" id="notesToggleBtn_'+idx+'" onclick="toggleNotes('+idx+')">'+(exp?'▴':'▾')+'</button></div><div class="notes-list" id="notesList_'+idx+'">'+entries+'<div class="notes-hint">blur \xB7 Enter = save \xB7 ◎ = set for channel</div></div></div>';}

function updateChannel(idx){
    const ch=channels[idx];
    let events=0;
    document.querySelectorAll(`.event-cb[data-ch="${idx}"]:checked`).forEach(cb=>{events|=(1<<parseInt(cb.dataset.hour));});
    ch.events=events;
    let days=0;
    document.querySelectorAll(`.day-cb[data-ch="${idx}"]:checked`).forEach(cb=>{days|=(1<<parseInt(cb.dataset.day));});
    ch.days=days;
    const doseInput=document.getElementById(`dose_${idx}`);
    ch.dailyDose=parseFloat(doseInput.value)||0;
    const evCnt=popcount(ch.events);
    const dayCnt=popcount(ch.days);
    const single=evCnt>0?ch.dailyDose/evCnt:0;
    const pumpTime=ch.dosingRate>0?single/ch.dosingRate:0;
    const weekly=ch.dailyDose*dayCnt;
    document.getElementById(`evInfo_${idx}`).textContent=`${evCnt} of 23`;
    document.getElementById(`dayInfo_${idx}`).textContent=`${dayCnt} of 7`;
    document.getElementById(`single_${idx}`).textContent=`${single.toFixed(1)} ml`;
    document.getElementById(`pumpTime_${idx}`).textContent=`${pumpTime.toFixed(1)} s`;
    document.getElementById(`weekly_${idx}`).textContent=`${weekly.toFixed(1)} ml`;
    
    const validMsg=document.getElementById(`validMsg_${idx}`);
    const validTxt=document.getElementById(`validTxt_${idx}`);
    const saveBtn=document.getElementById(`saveBtn_${idx}`);
    validMsg.className='valid-msg';
    let icon='';
    
    if(evCnt===0){validMsg.classList.add('info');validTxt.textContent='Select time slots to activate';icon='<circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/>';saveBtn.disabled=false;ch.state='inactive';}
    else if(dayCnt===0){validMsg.classList.add('warn');validTxt.textContent='Select active days';icon='<path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>';saveBtn.disabled=true;ch.state='incomplete';}
    else if(ch.dailyDose<=0){validMsg.classList.add('warn');validTxt.textContent='Enter daily dose';icon='<path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/>';saveBtn.disabled=true;ch.state='incomplete';}
    else if(single<CFG.MIN_DOSE_ML){validMsg.classList.add('err');validTxt.textContent=`Single dose ${single.toFixed(1)}ml < min ${CFG.MIN_DOSE_ML}ml`;icon='<circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/>';saveBtn.disabled=true;ch.state='invalid';}
    else if(pumpTime>CFG.MAX_PUMP_SEC){validMsg.classList.add('err');validTxt.textContent=`Pump time ${pumpTime.toFixed(0)}s > max ${CFG.MAX_PUMP_SEC}s`;icon='<circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/>';saveBtn.disabled=true;ch.state='invalid';}
    else{validMsg.classList.add('ok');validTxt.textContent='Configuration valid';icon='<path d="M22 11.08V12a10 10 0 11-5.93-9.14"/><polyline points="22,4 12,14.01 9,11.01"/>';saveBtn.disabled=false;ch.state='configured';}
    
    validMsg.querySelector('svg').innerHTML=icon;
    const badge=document.querySelector(`.channel-card[data-ch="${idx}"] .state-badge`);
    if(badge){badge.className=`state-badge ${ch.state}`;badge.textContent=getStateLabel(ch.state);}
    document.querySelectorAll(`.ch-nav-btn[data-ch="${idx}"]`).forEach(btn=>{btn.className=`ch-nav-btn ${ch.state}${currentCh===idx?' active':''}`;});
}

function runCalib(idx){
    const btn=document.getElementById(`calibBtn_${idx}`);
    btn.classList.add('running');
    btn.textContent='Running...';
    fetch(`api/calibrate?channel=${idx}`,{method:'POST'}).then(r=>r.json()).catch(()=>({}));
    let remaining=CFG.CALIB_SEC;
    const timer=setInterval(()=>{
        remaining--;
        btn.textContent=`${remaining}s`;
        if(remaining<=0){clearInterval(timer);btn.classList.remove('running');btn.textContent='Run Pump (30s)';}
    },1000);
}

function calcCalibration(idx){
    const input=document.getElementById(`calibMl_${idx}`);
    const val=input.value.trim();
    const validMsg=document.getElementById(`validMsg_${idx}`);
    const validTxt=document.getElementById(`validTxt_${idx}`);
    const saveBtn=document.getElementById(`saveBtn_${idx}`);
    const errIcon='<circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/>';
    if(val===''){updateChannel(idx);return;}
    const ml=parseFloat(val);
    if(isNaN(ml)||ml<=0){
        validMsg.className='valid-msg err';
        validTxt.textContent='Calibration: enter a positive value (ml pumped in 30s)';
        validMsg.querySelector('svg').innerHTML=errIcon;
        saveBtn.disabled=true;
        return;
    }
    const rate=ml/CFG.CALIB_SEC;
    if(rate<CFG.MIN_RATE||rate>CFG.MAX_RATE){
        validMsg.className='valid-msg err';
        const minMl=(CFG.MIN_RATE*CFG.CALIB_SEC).toFixed(1);
        const maxMl=(CFG.MAX_RATE*CFG.CALIB_SEC).toFixed(0);
        validTxt.textContent=`Calibration: ${ml} ml → ${rate.toFixed(3)} ml/s out of range (need ${minMl}–${maxMl} ml)`;
        validMsg.querySelector('svg').innerHTML=errIcon;
        saveBtn.disabled=true;
        return;
    }
    channels[idx].dosingRate=rate;
    updateChannel(idx);
}

function goToChannel(idx){
    document.querySelectorAll('.channel-card').forEach(card=>{card.classList.remove('active');});
    currentCh=idx;
    document.querySelectorAll('.ch-nav-btn').forEach(btn=>{btn.classList.toggle('active',parseInt(btn.dataset.ch)===idx);});
    const newCard=document.querySelector(`.channel-card[data-ch="${idx}"]`);
    if(newCard)newCard.classList.add('active');
}

function nextCh(){if(currentCh<CFG.CHANNEL_COUNT-1)goToChannel(currentCh+1);}
function prevCh(){if(currentCh>0)goToChannel(currentCh-1);}

function setupTouch(){
    const container=document.querySelector('.app-container');
    container.addEventListener('touchstart',e=>{touchStart.x=e.changedTouches[0].screenX;touchStart.y=e.changedTouches[0].screenY;},{passive:true});
    container.addEventListener('touchend',e=>{
        const dx=e.changedTouches[0].screenX-touchStart.x;
        const dy=e.changedTouches[0].screenY-touchStart.y;
        if(Math.abs(dx)>Math.abs(dy)&&Math.abs(dx)>CFG.SWIPE_THRESHOLD){if(dx<0)nextCh();else prevCh();}
    },{passive:true});
}

function showSessionExpired(){document.getElementById('sessionExpiredModal').classList.add('show');}
function loadStatus(){
    fetch('api/dosing-status').then(r=>{if(r.status===401){showSessionExpired();return null;}return r.json();}).then(data=>{if(!data)return;
        if(data.channels){
            data.channels.forEach((chData,i)=>{
                if(!channels[i])return;
                // Pola statusu — aktualizuj zawsze, niezależnie od trybu edycji
                channels[i].containerMl=chData.containerMl||1000;
                channels[i].remainingMl=chData.remainingMl!=null?chData.remainingMl:1000;
                channels[i].remainingPct=chData.remainingPct!=null?chData.remainingPct:100;
                channels[i].lowVolume=!!chData.lowVolume;
                channels[i].daysRemaining=chData.daysRemaining||999;
                channels[i].totalDosedMl=chData.totalDosedMl||0;
                // Pola konfiguracji — pomijaj kanał aktualnie edytowany
                if(i===editingChannel)return;
                channels[i].events=chData.events||0;
                channels[i].days=chData.days||0;
                channels[i].dailyDose=chData.dailyDose||0;
                channels[i].dosingRate=chData.dosingRate||0.33;
                channels[i].enabled=chData.enabled!==false;
                channels[i].name=chData.name||('CH'+i);
                channels[i].eventsCompleted=chData.eventsCompleted||0;
                channels[i].eventsFailed=chData.eventsFailed||0;
                channels[i].state=chData.state||'inactive';
            });
            if(editingChannel===-1&&!editingNotes){
                renderChannels();
            } else {
                // Re-render zablokowany (user edytuje) — odśwież tylko kontrolki wolumenu i dozowania
                channels.forEach((_,i)=>{
                    updateContainerDisplay(i,{remaining_ml:channels[i].remainingMl,remaining_pct:channels[i].remainingPct,container_ml:channels[i].containerMl,low_warning:channels[i].lowVolume,days_remaining:channels[i].daysRemaining});
                    updateDosedDisplay(i);
                });
            }
        }
        if(data.activeChannel!==undefined){
            const wasActive=activeChannel>=0;
            activeChannel=data.activeChannel;
            activeEventHour=data.activeEventHour!==undefined?data.activeEventHour:-1;
            if(wasActive!==(activeChannel>=0))renderChannels();
        }
    }).catch(()=>{});
}

function popcount(n){let count=0;while(n){count+=n&1;n>>>=1;}return count;}
function getStateLabel(state){return{inactive:'Inactive',incomplete:'Setup',invalid:'Invalid',configured:'Active',pending:'Pending'}[state]||state;}
function updateClock(){const now=new Date();const t=String(now.getUTCHours()).padStart(2,'0')+':'+String(now.getUTCMinutes()).padStart(2,'0')+':'+String(now.getUTCSeconds()).padStart(2,'0');document.querySelectorAll('.card-time').forEach(el=>el.textContent=t);}
function logout(){showConfirm('Logout','End current session?','logout',()=>{fetch('api/logout',{method:'POST'}).then(()=>{if(window.location.pathname.indexOf('/device/')!==-1){window.location.href='/dashboard';}else{window.location.href='login';}});});}

function saveContainerSize(idx){
    const input=document.getElementById(`container_${idx}`);
    const ml=parseInt(input.value)||1000;
    const clamped=Math.max(100,Math.min(5000,ml));
    input.value=clamped;
    fetch('api/container-volume',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({channel:idx,container_ml:clamped})})
    .then(r=>r.json()).then(data=>{if(data.success)updateContainerDisplay(idx,data);}).catch(err=>console.error('Save container error:',err));
}

function showRefillModal(idx){
    pendingRefillChannel=idx;
    const ch=channels[idx];
    document.getElementById('modalChannel').textContent=idx+1;
    document.getElementById('modalCapacity').textContent=(ch.containerMl||1000).toFixed(0);
    document.getElementById('refillModal').classList.add('show');
}

function closeRefillModal(){document.getElementById('refillModal').classList.remove('show');pendingRefillChannel=-1;}

function confirmRefill(){
    if(pendingRefillChannel<0){closeRefillModal();return;}
    const channel=pendingRefillChannel;
    closeRefillModal();
    fetch(`api/refill?channel=${channel}`,{method:'POST'}).then(r=>r.json()).then(data=>{
        if(data.success)updateContainerDisplay(channel,data);
    }).catch(err=>console.error('Refill error:',err));
}

let pendingSaveChannel=-1;

function showSaveModal(idx){
    pendingSaveChannel=idx;
    document.getElementById('saveModalChannel').textContent=idx+1;
    document.getElementById('saveModal').classList.add('show');
}

function closeSaveModal(){document.getElementById('saveModal').classList.remove('show');pendingSaveChannel=-1;}

function confirmSave(){
    if(pendingSaveChannel<0){closeSaveModal();return;}
    const idx=pendingSaveChannel;
    closeSaveModal();
    const ch=channels[idx];
    const payload={channel:idx,events:ch.events,days:ch.days,dailyDose:ch.dailyDose,dosingRate:ch.dosingRate,enabled:ch.enabled!==false};
    fetch('api/dosing-config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)})
    .then(r=>r.json())
    .then(data=>{editingChannel=-1;if(data.success){ch.state='pending';renderChannels();showAlert('Success','Configuration saved. Changes active from tomorrow.','ok');}else{showAlert('Error','Save failed: '+(data.error||'Unknown error'),'err');}})
    .catch(()=>showAlert('Error','Connection error','err'));
}

function resetDosed(idx){
    fetch(`api/reset-dosed?channel=${idx}`,{method:'POST'}).then(r=>r.json()).then(data=>{
        if(data.success){
            channels[idx].totalDosedMl=0;
            updateDosedDisplay(idx);
        }
    }).catch(err=>console.error('Reset dosed error:',err));
}

function updateDosedDisplay(idx){
    const ch=channels[idx];
    const weekly=ch.dailyDose*popcount(ch.days);
    const dosedPct=weekly>0?Math.min(100,(ch.totalDosedMl/weekly)*100):0;
    const dosedBar=document.getElementById(`dosedBar_${idx}`);
    const dosedLabel=document.getElementById(`dosedLabel_${idx}`);
    if(dosedBar)dosedBar.style.width=`${dosedPct}%`;
    if(dosedLabel)dosedLabel.textContent=ch.totalDosedMl.toFixed(1);
}

function updateContainerDisplay(idx,data){
    const bar=document.getElementById(`containerBar_${idx}`);
    const remainingLabel=document.getElementById(`remainingLabel_${idx}`);
    const daysLeftEl=document.getElementById(`daysLeft_${idx}`);
    if(bar){const pctVal=data.remaining_pct!==undefined?data.remaining_pct:(data.remaining_ml/data.container_ml*100);bar.style.width=`${pctVal}%`;bar.classList.toggle('low',data.low_warning);}
    if(remainingLabel){remainingLabel.textContent=data.remaining_ml.toFixed(0);remainingLabel.classList.toggle('low',data.low_warning);}
    if(daysLeftEl&&data.days_remaining!==undefined){daysLeftEl.textContent=data.days_remaining>900?'∞':data.days_remaining.toFixed(1);}
    if(channels[idx]){channels[idx].containerMl=data.container_ml;channels[idx].remainingMl=data.remaining_ml;channels[idx].remainingPct=data.remaining_pct;channels[idx].lowVolume=data.low_warning;if(data.days_remaining!==undefined)channels[idx].daysRemaining=data.days_remaining;}
}

document.addEventListener('click',function(e){if(e.target.id==='refillModal')closeRefillModal();if(e.target.id==='alertModal')closeAlert();if(e.target.id==='saveModal')closeSaveModal();});
document.addEventListener('keydown',function(e){if(e.key==='Escape'){closeRefillModal();closeAlert();closeSaveModal();}});
const MODAL_ICONS={
ok:'<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M22 11.08V12a10 10 0 11-5.93-9.14"/><polyline points="22,4 12,14.01 9,11.01"/></svg>',
err:'<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>',
warn:'<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>',
info:'<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/></svg>',
logout:'<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M9 21H5a2 2 0 01-2-2V5a2 2 0 012-2h4"/><polyline points="16,17 21,12 16,7"/><line x1="21" y1="12" x2="9" y2="12"/></svg>'
};
let alertCallback=null;let alertTimeout=null;

function showAlert(title,msg,type,callback){
    document.getElementById('alertIcon').className='modal-icon '+type;
    document.getElementById('alertIcon').innerHTML=MODAL_ICONS[type]||MODAL_ICONS.info;
    document.getElementById('alertTitle').textContent=title;
    document.getElementById('alertText').textContent=msg;
    alertCallback=callback||null;
    if(type==='ok'){
        document.getElementById('alertActions').innerHTML='';
        alertTimeout=setTimeout(closeAlert,1000);
    }else{
        document.getElementById('alertActions').innerHTML='<button class="btn btn-primary" onclick="closeAlert()">OK</button>';
    }
    document.getElementById('alertModal').classList.add('show');
}

function showConfirm(title,msg,type,onConfirm){
    document.getElementById('alertIcon').className='modal-icon '+type;
    document.getElementById('alertIcon').innerHTML=MODAL_ICONS[type]||MODAL_ICONS.info;
    document.getElementById('alertTitle').textContent=title;
    document.getElementById('alertText').textContent=msg;
    document.getElementById('alertActions').innerHTML='<button class="btn btn-secondary" onclick="closeAlert()">Cancel</button><button class="btn btn-primary" onclick="closeAlert(true)">Confirm</button>';
    alertCallback=onConfirm;
    document.getElementById('alertModal').classList.add('show');
}

function closeAlert(confirmed){
    if(alertTimeout){clearTimeout(alertTimeout);alertTimeout=null;}
    document.getElementById('alertModal').classList.remove('show');
    if(confirmed&&alertCallback)alertCallback();
    alertCallback=null;
}
// ── ParamLog CRUD ────────────────────────────────────────────────────────
function loadParamLog(){
    fetch('api/paramlog').then(r=>r.json()).then(data=>{
        paramLog=data;
        for(let c=0;c<8;c++) chTmplAssign[c]=[];
        getValidRecs().forEach(r=>{if(!chTmplAssign[r.channel].includes(r.tmpl_idx))chTmplAssign[r.channel].push(r.tmpl_idx);});
        for(let c=0;c<CFG.CHANNEL_COUNT;c++) refreshParams(c);
    }).catch(()=>{});
}
function saveParamLog(cb){
    fetch('api/paramlog',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(paramLog)})
    .then(r=>r.json()).then(d=>{if(cb)cb(!!d.ok);}).catch(()=>{if(cb)cb(false);});
}
function showAddRec(ch,tmplId){
    const f=document.getElementById('addForm_'+ch+'_'+tmplId);
    if(f){f.classList.add('visible');const inp=f.querySelector('.add-rec-input');if(inp)inp.focus();}
}
function hideAddRec(ch,tmplId){
    const f=document.getElementById('addForm_'+ch+'_'+tmplId);
    if(f){f.classList.remove('visible');const inp=f.querySelector('.add-rec-input');if(inp)inp.value='';}
}
function saveRec(ch,tmplId){
    if(!paramLog) return;
    const inp=document.getElementById('addVal_'+ch+'_'+tmplId);
    if(!inp) return;
    const val=parseFloat(inp.value);
    if(isNaN(val)){inp.focus();return;}
    const now=Math.floor(Date.now()/1000);
    paramLog.ring[paramLog.head]={value:val,timestamp:now,tmpl_idx:tmplId,channel:ch,flags:1};
    paramLog.head=(paramLog.head+1)%100;
    if(paramLog.count<100) paramLog.count++;
    hideAddRec(ch,tmplId);
    saveParamLog(()=>refreshParams(ch));
}
function editRec(ri,ch){
    if(!paramLog||!paramLog.ring[ri]) return;
    const span=document.getElementById('rv_'+ri);if(!span) return;
    const r=paramLog.ring[ri];
    const inp=document.createElement('input');
    inp.type='number';inp.step='any';inp.className='rec-value-input';inp.value=r.value;
    span.replaceWith(inp);inp.focus();inp.select();
    const commit=()=>{
        const v=parseFloat(inp.value);
        if(!isNaN(v)){r.value=v;r.timestamp=Math.floor(Date.now()/1000);saveParamLog(()=>refreshParams(ch));}
        else refreshParams(ch);
    };
    inp.addEventListener('blur',commit);
    inp.addEventListener('keydown',e=>{
        if(e.key==='Enter'){inp.removeEventListener('blur',commit);commit();e.preventDefault();}
        if(e.key==='Escape'){inp.removeEventListener('blur',commit);refreshParams(ch);}
    });
}
function deleteRec(ri,ch){
    if(!paramLog||!paramLog.ring[ri]) return;
    paramLog.ring[ri].flags=0;
    saveParamLog(()=>refreshParams(ch));
}
function assignTmpl(ch){
    const sel=document.getElementById('assignSel_'+ch);
    if(!sel||!sel.value) return;
    const id=parseInt(sel.value);
    if(!chTmplAssign[ch].includes(id)) chTmplAssign[ch].push(id);
    refreshParams(ch);
}
function removeTmpl(ch,tmplId){
    chTmplAssign[ch]=chTmplAssign[ch].filter(id=>id!==tmplId);
    const cs=getChartState(ch,tmplId);cs.open=false;
    refreshParams(ch);
}
function showNewTmplForm(ch){
    const btn=document.getElementById('newTmplBtn_'+ch);if(btn)btn.style.display='none';
    const f=document.getElementById('newTmplForm_'+ch);
    if(f){f.classList.add('visible');const n=f.querySelector('.new-tmpl-name');if(n)n.focus();}
}
function hideNewTmplForm(ch){
    const btn=document.getElementById('newTmplBtn_'+ch);if(btn)btn.style.display='';
    const f=document.getElementById('newTmplForm_'+ch);
    if(f){f.classList.remove('visible');const n=f.querySelector('.new-tmpl-name');if(n)n.value='';const u=f.querySelector('.new-tmpl-unit');if(u)u.value='';}
}
function createTmpl(ch){
    if(!paramLog) return;
    const nameEl=document.getElementById('newTmplName_'+ch);
    const unitEl=document.getElementById('newTmplUnit_'+ch);
    const name=(nameEl?nameEl.value:'').trim();
    if(!name){if(nameEl)nameEl.focus();return;}
    if(paramLog.tmpl_count>=20){showAlert('Limit','Max 20 templates reached','warn');return;}
    let slot=-1;
    for(let i=0;i<20;i++){if(!(paramLog.templates[i].flags&1)){slot=i;break;}}
    if(slot<0){showAlert('Full','No free template slots','warn');return;}
    paramLog.templates[slot]={name:name,unit:(unitEl?unitEl.value:'').trim(),flags:1};
    paramLog.tmpl_count++;
    if(!chTmplAssign[ch].includes(slot)) chTmplAssign[ch].push(slot);
    hideNewTmplForm(ch);
    saveParamLog(()=>refreshParams(ch));
}
// ── ParamLog helpers ─────────────────────────────────────────────────────
function getChartState(ch,tmplId){
    const k=ch+'-'+tmplId;
    if(!chartState[k]) chartState[k]={open:false,offset:0};
    return chartState[k];
}
function chartWindow(){return window.matchMedia('(max-width:600px)').matches?20:80;}
function getValidRecs(){
    if(!paramLog) return [];
    return paramLog.ring.map((r,i)=>Object.assign({},r,{_ri:i})).filter(r=>r.flags&1).sort((a,b)=>a.timestamp-b.timestamp);
}
function getChartRecs(ch,tmplId){return getValidRecs().filter(r=>r.tmpl_idx===tmplId&&r.channel===ch);}
function fmtTs(ts){return new Date(ts*1000).toISOString().slice(0,16).replace('T',' ');}

function toggleParams(idx){
    paramsExpanded[idx]=!paramsExpanded[idx];
    const s=document.getElementById('paramsSec_'+idx);
    const b=document.getElementById('paramsToggleBtn_'+idx);
    if(s) s.classList.toggle('expanded',paramsExpanded[idx]);
    if(b) b.textContent=paramsExpanded[idx]?'▴':'▾';
}
function refreshParams(idx){
    const old=document.getElementById('paramsSec_'+idx);
    if(!old) return;
    const tmp=document.createElement('div');
    tmp.innerHTML=renderParamsSection(idx);
    old.parentNode.replaceChild(tmp.firstElementChild,old);
    chTmplAssign[idx].forEach(tmplId=>{
        if(getChartState(idx,tmplId).open) requestAnimationFrame(()=>drawChart(idx,tmplId));
    });
}
function renderParamsSection(idx){
    const exp=paramsExpanded[idx];
    const svgBar='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="18" y1="20" x2="18" y2="10"/><line x1="12" y1="20" x2="12" y2="4"/><line x1="6" y1="20" x2="6" y2="14"/></svg>';
    const svgWave='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22,12 18,12 15,20 9,4 6,12 2,12"/></svg>';
    const svgDown='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>';
    const hdr='<div class="params-header"><svg class="params-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="18" y1="20" x2="18" y2="10"/><line x1="12" y1="20" x2="12" y2="4"/><line x1="6" y1="20" x2="6" y2="14"/></svg>';
    if(!paramLog){
        return '<div class="params-section" id="paramsSec_'+idx+'">'+hdr+'<span class="params-preview empty">Loading parameters…</span></div></div>';
    }
    const ids=chTmplAssign[idx];
    const chRecs=getValidRecs().filter(r=>r.channel===idx);
    const empty=ids.length===0;
    const summary=empty?'— no templates assigned':ids.length+' template'+(ids.length!==1?'s':'')+' \xb7 '+chRecs.length+' record'+(chRecs.length!==1?'s':'');
    let tmplHtml=empty
        ?'<div class="params-empty">No templates assigned — use controls below</div>'
        :ids.map(function(tmplId){
            const t=paramLog.templates[tmplId];
            if(!t||(t.flags&1)===0) return '';
            const ud=t.unit||'—';
            const cs=getChartState(idx,tmplId);
            const win=chartWindow();
            const allRecs=getChartRecs(idx,tmplId);
            const prevDis=!cs.open||cs.offset===0;
            const nextDis=!cs.open||cs.offset+win>=allRecs.length;
            const recRows=allRecs.length===0
                ?'<div class="records-empty">No measurements yet — click + Add</div>'
                :allRecs.slice().reverse().map(function(r){
                    return '<div class="record-row">'
                        +'<span class="rec-value" id="rv_'+r._ri+'">'+r.value+'</span>'
                        +'<span class="rec-ts">'+fmtTs(r.timestamp)+' UTC</span>'
                        +'<div class="rec-actions">'
                        +'<button class="rec-btn" onclick="editRec('+r._ri+','+idx+')" title="Edit">✎</button>'
                        +'<button class="rec-btn del" onclick="deleteRec('+r._ri+','+idx+')" title="Delete">\xd7</button>'
                        +'</div></div>';
                }).join('');
            return '<div class="tmpl-block">'
                +'<div class="tmpl-header">'
                +'<span class="tmpl-dot"></span>'
                +'<span class="tmpl-name">'+escAttr(t.name)+'</span>'
                +'<span class="tmpl-unit">'+escAttr(ud)+'</span>'
                +'<button class="tmpl-icon-btn'+(cs.open?' active':'')+'" id="chartBtn_'+idx+'_'+tmplId+'" onclick="toggleChart('+idx+','+tmplId+')" title="Toggle chart">'+svgWave+'</button>'
                +'<button class="tmpl-icon-btn csv" title="Export CSV">'+svgDown+'</button>'
                +'<button class="tmpl-add-btn" onclick="showAddRec('+idx+','+tmplId+')">+ Add</button>'
                +'<button class="tmpl-remove-btn" onclick="removeTmpl('+idx+','+tmplId+')" title="Remove">\xd7</button>'
                +'</div>'
                +'<div class="chart-container" id="chart_'+idx+'_'+tmplId+'" style="display:'+(cs.open?'block':'none')+'">'
                +'<div class="chart-wrap">'
                +'<button class="chart-scroll-btn" id="chartPrev_'+idx+'_'+tmplId+'" onclick="scrollChart('+idx+','+tmplId+',-1)"'+(prevDis?' disabled':'')+'>&#9668;</button>'
                +'<canvas class="chart-canvas" id="canvas_'+idx+'_'+tmplId+'"></canvas>'
                +'<button class="chart-scroll-btn" id="chartNext_'+idx+'_'+tmplId+'" onclick="scrollChart('+idx+','+tmplId+',1)"'+(nextDis?' disabled':'')+'>&#9658;</button>'
                +'</div><div class="chart-footer" id="chartFoot_'+idx+'_'+tmplId+'"></div></div>'
                +'<div class="tmpl-records">'
                +'<div class="add-record-form" id="addForm_'+idx+'_'+tmplId+'">'
                +'<input class="add-rec-input" id="addVal_'+idx+'_'+tmplId+'" type="number" step="any" placeholder="value"'
                +' onkeydown="if(event.key===\'Enter\')saveRec('+idx+','+tmplId+');if(event.key===\'Escape\')hideAddRec('+idx+','+tmplId+')">'
                +'<span class="add-rec-unit">'+escAttr(ud)+'</span>'
                +'<button class="add-rec-save" onclick="saveRec('+idx+','+tmplId+')">Save</button>'
                +'<button class="add-rec-cancel" onclick="hideAddRec('+idx+','+tmplId+')">\xd7</button>'
                +'</div>'+recRows+'</div></div>';
        }).join('');
    const available=paramLog.templates.map(function(t,i){return Object.assign({},t,{_i:i});}).filter(function(t){return (t.flags&1)&&!ids.includes(t._i);});
    const assignHtml=available.length
        ?'<div class="assign-row"><select class="assign-select" id="assignSel_'+idx+'"><option value="">— assign template —</option>'
          +available.map(function(t){return '<option value="'+t._i+'">'+escAttr(t.name)+(t.unit?' ('+escAttr(t.unit)+')':'')+'</option>';}).join('')
          +'</select><button class="assign-btn" onclick="assignTmpl('+idx+')">Assign</button></div>'
        :'';
    return '<div class="params-section'+(exp?' expanded':'')+'" id="paramsSec_'+idx+'">'
        +hdr
        +'<span class="params-preview'+(empty?' empty':'')+'" id="paramsPreview_'+idx+'">'+summary+'</span>'
        +'<button class="params-toggle-btn" id="paramsToggleBtn_'+idx+'" onclick="toggleParams('+idx+')">'+(exp?'▴':'▾')+'</button>'
        +'</div>'
        +'<div class="params-body" id="paramsBody_'+idx+'">'
        +tmplHtml
        +'<div class="params-bottom">'
        +assignHtml
        +'<button class="new-tmpl-btn" id="newTmplBtn_'+idx+'" onclick="showNewTmplForm('+idx+')">'
        +'+ New Template \xb7 ('+paramLog.tmpl_count+'/20 \xb7 ring: '+paramLog.count+'/100)</button>'
        +'<div class="new-tmpl-form" id="newTmplForm_'+idx+'">'
        +'<input class="new-tmpl-name" id="newTmplName_'+idx+'" type="text" placeholder="Parameter name" maxlength="20"'
        +' onkeydown="if(event.key===\'Enter\')createTmpl('+idx+');if(event.key===\'Escape\')hideNewTmplForm('+idx+')">'
        +'<input class="new-tmpl-unit" id="newTmplUnit_'+idx+'" type="text" placeholder="unit" maxlength="8"'
        +' onkeydown="if(event.key===\'Enter\')createTmpl('+idx+');if(event.key===\'Escape\')hideNewTmplForm('+idx+')">'
        +'<button class="new-tmpl-create" onclick="createTmpl('+idx+')">Create</button>'
        +'<button class="new-tmpl-cancel" onclick="hideNewTmplForm('+idx+')">\xd7</button>'
        +'</div></div></div></div>';
}

document.addEventListener('DOMContentLoaded',init);
</script>
</body>
</html>
)rawliteral";

String getLoginHTML(){return String(LOGIN_HTML);}
String getDashboardHTML(){return String(DASHBOARD_HTML);}