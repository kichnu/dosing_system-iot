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
--card-width:600px;--card-padding:12px;--section-gap:10px;--section-padding:10px;
--event-slot-height:38px;--day-slot-height:36px;--input-height:38px;--btn-height:40px;--header-height:44px;--footer-height:56px;
--font-xs:0.6rem;--font-sm:0.7rem;--font-md:0.8rem;--font-lg:0.95rem;
--swipe-threshold:50px
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
.card-inner{width:100%;max-width:var(--card-width);height:100%;display:flex;flex-direction:column;background:var(--bg-card);border-left:1px solid var(--border);border-right:1px solid var(--border)}
.card-body{flex:1;overflow-y:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.4) transparent}
.card-body::-webkit-scrollbar{width:8px}
.card-body::-webkit-scrollbar-thumb{background-color:rgba(255,255,255,0.25);border-radius:6px}

.card-header{display:flex;align-items:center;justify-content:space-between;padding:10px var(--card-padding);background:var(--bg-card);border-bottom:1px solid var(--border);height:var(--header-height);flex-shrink:0}
.channel-title{display:flex;align-items:center;gap:8px}
.channel-number{margin-left:40px;margin-right:10px;font-size:22px;font-weight:700}
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

.events-grid{display:grid;grid-template-columns:repeat(8,1fr);gap:4px}
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

.volume-row{display:flex;align-items:flex-end;gap:8px;margin-bottom:10px}
.volume-group{flex:1}
.volume-label{display:block;font-size:var(--font-xs);font-weight:600;text-transform:uppercase;letter-spacing:0.05em;color:var(--text-muted);margin-bottom:4px}
.volume-input{width:100%;height:var(--input-height);padding:0 10px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-lg);font-weight:600;color:var(--text-primary);text-align:center}
.volume-input:focus{outline:none;border-color:var(--accent-cyan);box-shadow:0 0 0 2px rgba(34,211,213,0.15)}
.volume-unit{font-size:var(--font-sm);font-weight:600;color:var(--text-muted);padding-bottom:10px}

.calc-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:6px}
.calc-item{background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);padding:8px;text-align:center}
.calc-lbl{font-size:var(--font-xs);font-weight:600;text-transform:uppercase;letter-spacing:0.05em;color:var(--text-muted);margin-bottom:2px}
.calc-val{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-md);font-weight:700;color:var(--text-primary)}
.calc-item.hl .calc-val{color:var(--accent-cyan)}
.calc-item.ok .calc-val{color:var(--accent-green)}
.calc-item.warn .calc-val{color:var(--accent-yellow)}

.calib-row{display:flex;gap:8px;align-items:flex-end}
.calib-btn{flex:1;display:flex;align-items:center;justify-content:center;gap:6px;height:var(--btn-height);background:rgba(34,211,213,0.1);border:1px solid var(--accent-cyan);border-radius:var(--radius-sm);color:var(--accent-cyan);font-size:var(--font-sm);font-weight:600;cursor:pointer;transition:all var(--transition-fast)}
.calib-btn:hover{background:rgba(34,211,213,0.2)}
.calib-btn:active{transform:scale(0.98)}
.calib-btn.running{background:rgba(234,179,8,0.15);border-color:var(--accent-yellow);color:var(--accent-yellow);pointer-events:none}
.calib-btn svg{width:14px;height:14px}
.calib-input-group{width:80px}
.calib-label{display:block;font-size:var(--font-xs);font-weight:600;text-transform:uppercase;color:var(--text-muted);margin-bottom:4px}
.calib-input{width:100%;height:var(--input-height);padding:0 6px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-md);font-weight:600;color:var(--text-primary);text-align:center}
.calib-input:focus{outline:none;border-color:var(--accent-cyan)}

.valid-msg{display:flex;align-items:center;gap:6px;padding:8px 10px;border-radius:var(--radius-sm);font-size:var(--font-sm);font-weight:500;flex-shrink:0}
.valid-msg svg{width:14px;height:14px;flex-shrink:0}
.valid-msg.err{background:rgba(239,68,68,0.1);border:1px solid rgba(239,68,68,0.3);color:var(--accent-red)}
.valid-msg.warn{background:rgba(234,179,8,0.1);border:1px solid rgba(234,179,8,0.3);color:var(--accent-yellow)}
.valid-msg.info{background:rgba(56,189,248,0.1);border:1px solid rgba(56,189,248,0.3);color:var(--accent-blue)}
.valid-msg.ok{background:rgba(34,197,94,0.1);border:1px solid rgba(34,197,94,0.3);color:var(--accent-green)}

.card-footer{padding:10px var(--card-padding);background:var(--bg-card);border-top:1px solid var(--border);display:flex;margin-bottom:6px;gap:8px;height:var(--footer-height);flex-shrink:0}
.btn{flex:1;height:var(--btn-height);border-radius:var(--radius-sm);font-size:var(--font-sm);font-weight:600;cursor:pointer;transition:all var(--transition-fast);display:flex;align-items:center;justify-content:center;gap:5px}
.btn svg{width:14px;height:14px}
.btn-primary{background:linear-gradient(135deg,var(--accent-cyan),var(--accent-blue));border:none;color:var(--bg-primary)}
.btn-primary:hover{transform:translateY(-1px);box-shadow:0 4px 12px rgba(34,211,213,0.3)}
.btn-primary:disabled{opacity:0.5;cursor:not-allowed;transform:none;box-shadow:none}
.btn-secondary{background:var(--bg-input);border:1px solid var(--border);color:var(--text-secondary)}
.btn-secondary:hover{background:var(--bg-card-hover);color:var(--text-primary)}

.pending-banner{display:flex;align-items:center;border-top-left-radius:var(--radius-lg);border-top-right-radius:var(--radius-lg);gap:6px;height:32px;width:100%;padding:8px var(--card-padding);background:rgba(56,189,248,0.1);border-bottom:1px solid rgba(56,189,248,0.3);font-size:var(--font-sm);color:var(--accent-blue);flex-shrink:0}
.pending-banner svg{width:14px;height:14px}

@media (max-width:768px) and (hover:none){
.channel-number{margin-left:10px}
.card-header{height:auto}
.channel-nav{grid-template-columns:repeat(4,1fr)}
.card-inner{border-left:none;border-right:none}
}

/* Volume group full width */
.volume-group.full-width{width:100%;margin-bottom:10px}
.volume-group.full-width .volume-input{width:100%}

/* Calc item full width (for Days Left) */
.calc-item-full{width:100%;height:var(--input-height);background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-lg);font-weight:700;color:var(--text-primary);display:flex;align-items:center;justify-content:center}

/* Save button in calc-grid */
.calc-grid .save-btn{background:linear-gradient(135deg,var(--accent-cyan),var(--accent-blue));border:none;color:var(--bg-primary);border-radius:var(--radius-sm);font-size:var(--font-sm);font-weight:600;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:5px;transition:all var(--transition-fast);height:49px}
.calc-grid .save-btn:hover{transform:translateY(-1px);box-shadow:0 4px 12px rgba(34,211,213,0.3)}
.calc-grid .save-btn:disabled{opacity:0.5;cursor:not-allowed;transform:none;box-shadow:none}
.calc-grid .save-btn svg{width:14px;height:14px}

/* Container Volume bars */
.bar-group{margin-bottom:10px}
.bar-row{display:flex;align-items:center;gap:8px;margin-bottom:6px;flex-direction:row-reverse}
.bar-row .bar-label{font-family:'SF Mono','Fira Code',monospace;font-size:var(--font-sm);font-weight:600;text-align:left;color:var(--text-secondary)}
.bar-row .bar-label.low{color:var(--accent-red)}
.container-bar{flex:1;height:10px;background:var(--bg-input);border:1px solid var(--border);border-radius:var(--radius-sm);overflow:hidden;position:relative}
.container-bar-fill{height:100%;background:linear-gradient(90deg,var(--accent-cyan),var(--accent-blue));transition:width 0.3s ease;border-radius:var(--radius-sm) 0 0 var(--radius-sm)}
.container-bar-fill.low{background:linear-gradient(90deg,var(--accent-red),var(--accent-orange))}
.container-bar-fill.dosed{background:linear-gradient(90deg,var(--accent-blue),var(--accent-cyan))}

/* Buttons row */
.buttons-row{display:flex;gap:8px;margin-top:10px}
.buttons-row .btn{flex:1}
.buttons-row .refill-btn,.buttons-row .reset-btn{margin-top:0}
.refill-btn svg,.reset-btn svg{width:14px;height:14px}

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
const CFG={CHANNEL_COUNT:8,EVENTS_PER_DAY:11,FIRST_EVENT_HOUR:2,LAST_EVENT_HOUR:22,CHANNEL_OFFSET_MIN:15,EVENT_WINDOW_SEC:300,MAX_PUMP_SEC:180,MIN_DOSE_ML:0.1,CALIB_SEC:30,SWIPE_THRESHOLD:50};
const DAYS=['Mon','Tue','Wed','Thu','Fri','Sat','Sun'];
const DAY_NAMES=['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];

let channels=[];
let currentCh=0;
let activeChannel=-1;
let activeEventHour=-1;
let touchStart={x:0,y:0};
let editingChannel=-1;
let pendingRefillChannel=-1;

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
}


function renderChannels(){
    const slider=document.getElementById('channelScreen');
    const activeCard=document.querySelector('.channel-card.active');
    const scrollTop=activeCard?activeCard.scrollTop:0;
    slider.innerHTML=channels.map((ch,idx)=>renderChannelCard(ch,idx)).join('');
    channels.forEach((ch,idx)=>{attachCardEvents(idx);});
    const newActiveCard=document.querySelector(`.channel-card[data-ch="${currentCh}"]`);
    if(newActiveCard){newActiveCard.classList.add('active');newActiveCard.scrollTop=scrollTop;}
}

function getNextEventHour(ch,chIdx){
    const now=new Date();
    const utcHour=now.getUTCHours();
    const utcMinute=now.getUTCMinutes();
    const channelOffset=chIdx*CFG.CHANNEL_OFFSET_MIN;
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
    const completedCnt=popcount(ch.eventsCompleted);
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
<div class="card-topbar"><div class="logo"><div class="logo-icon"><svg viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg></div><div class="logo-text">DOZOWNIK</div></div><span class="card-time">--:--:--</span><div class="header-actions"><button class="btn-icon" title="Logout" onclick="logout()"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M9 21H5a2 2 0 01-2-2V5a2 2 0 012-2h4"/><polyline points="16,17 21,12 16,7"/><line x1="21" y1="12" x2="9" y2="12"/></svg></button></div></div>
<div class="channel-nav">${navBtns}</div>
${ch.state==='pending'?`<div class="pending-banner"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><polyline points="12,6 12,12 16,14"/></svg>Changes pending – active from tomorrow</div>`:''}
<div class="card-header"><div class="channel-title"><span class="channel-number">${ch.name||('CH'+idx)}</span><span class="state-badge ${ch.state}">${getStateLabel(ch.state)}</span></div><button class="btn-toggle-enable ${ch.enabled?'on':'off'}" onclick="toggleChannelEnabled(${idx})" title="${ch.enabled?'Disable channel':'Enable channel'}">${ch.enabled?'ON':'OFF'}</button></div>
<div class="card-body">
<div class="card-content">
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><polyline points="12,6 12,12 16,14"/></svg>Time Schedule (UTC)</div><div class="section-info" id="evInfo_${idx}">${evCnt} of ${CFG.EVENTS_PER_DAY}</div></div><div class="section-body"><div class="events-grid">${eventsHtml}</div></div></div>
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><rect x="3" y="4" width="18" height="18" rx="2" ry="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/></svg>Active Days</div><div class="section-info" id="dayInfo_${idx}">${dayCnt} of 7</div></div><div class="section-body"><div class="days-grid">${daysHtml}</div></div></div>
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>Dosing Volume</div></div><div class="section-body"><div class="volume-group full-width"><label class="volume-label">Daily Dose (ml)</label><input type="number" class="volume-input" id="dose_${idx}" value="${ch.dailyDose}" step="0.1" min="0" data-ch="${idx}"></div><div class="calc-grid"><div class="calc-item hl"><div class="calc-lbl">Single Dose</div><div class="calc-val" id="single_${idx}">${single.toFixed(1)} ml</div></div><div class="calc-item"><div class="calc-lbl">Pump Time</div><div class="calc-val" id="pumpTime_${idx}">${pumpTime.toFixed(1)} s</div></div><div class="calc-item"><div class="calc-lbl">Weekly</div><div class="calc-val" id="weekly_${idx}">${weekly.toFixed(1)} ml</div></div><button class="btn btn-primary save-btn" id="saveBtn_${idx}" onclick="showSaveModal(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 01-2-2V5a2 2 0 012-2h11l5 5v11a2 2 0 01-2 2z"/><polyline points="17,21 17,13 7,13 7,21"/><polyline points="7,3 7,8 15,8"/></svg>Save</button></div></div></div>
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>Container Volume</div></div><div class="section-body"><div class="volume-group full-width"><label class="volume-label">Container Size (ml)</label><input type="number" class="volume-input" id="container_${idx}" value="${ch.containerMl||1000}" step="50" min="100" max="5000" onchange="saveContainerSize(${idx})"></div><div class="volume-group full-width"><label class="volume-label">Days Left</label><div class="calc-item-full" id="daysLeft_${idx}">${ch.daysRemaining?ch.daysRemaining.toFixed(1):'∞'}</div></div><div class="bar-group"><div class="bar-row"><span class="bar-label ${ch.lowVolume?'low':''}" id="remainingLabel_${idx}">remaining ${(ch.remainingMl||1000).toFixed(0)} ml</span><div class="container-bar"><div class="container-bar-fill ${ch.lowVolume?'low':''}" id="containerBar_${idx}" style="width:${ch.remainingPct||100}%"></div></div></div><div class="bar-row"><span class="bar-label" id="dosedLabel_${idx}">dosed ${(ch.totalDosedMl||0).toFixed(1)} ml</span><div class="container-bar"><div class="container-bar-fill dosed" id="dosedBar_${idx}" style="width:${weekly>0?Math.min(100,(ch.totalDosedMl||0)/weekly*100):0}%"></div></div></div></div><div class="buttons-row"><button class="btn btn-primary refill-btn" onclick="showRefillModal(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>Refill Container</button><button class="btn btn-primary reset-btn" onclick="resetDosed(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M3 12a9 9 0 109-9 9.75 9.75 0 00-6.74 2.74L3 8"/><path d="M3 3v5h5"/></svg>Reset Dosed</button></div></div></div>
<div class="section"><div class="section-header"><div class="section-title"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M14.7 6.3a1 1 0 000 1.4l1.6 1.6a1 1 0 001.4 0l3.77-3.77a6 6 0 01-7.94 7.94l-6.91 6.91a2.12 2.12 0 01-3-3l6.91-6.91a6 6 0 017.94-7.94l-3.76 3.76z"/></svg>Calibration</div></div><div class="section-body"><div class="calib-row"><button class="calib-btn" id="calibBtn_${idx}" onclick="runCalib(${idx})"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><polygon points="5,3 19,12 5,21 5,3"/></svg>Run Pump (30s)</button><div class="calib-input-group"><label class="calib-label">Measured</label><input type="number" class="calib-input" id="calibMl_${idx}" placeholder="ml" step="0.1" data-ch="${idx}"></div></div></div></div>
<div class="valid-msg ${validClass}" id="validMsg_${idx}"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24">${validIcon}</svg><span id="validTxt_${idx}">${validMsg}</span></div>
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
}

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
    const completedCnt=popcount(ch.eventsCompleted);
    const failedCnt=popcount(ch.eventsFailed||0);
    
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
    btn.innerHTML=`<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><polyline points="12,6 12,12 16,14"/></svg> Running...`;
    fetch(`api/calibrate?channel=${idx}`,{method:'POST'}).then(r=>r.json()).catch(()=>({}));
    let remaining=CFG.CALIB_SEC;
    const timer=setInterval(()=>{
        remaining--;
        btn.innerHTML=`<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><polyline points="12,6 12,12 16,14"/></svg> ${remaining}s`;
        if(remaining<=0){clearInterval(timer);btn.classList.remove('running');btn.innerHTML=`<svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><polygon points="5,3 19,12 5,21 5,3"/></svg> Run Pump (30s)`;}
    },1000);
}

function calcCalibration(idx){const ml=parseFloat(document.getElementById(`calibMl_${idx}`).value)||0;if(ml>0){channels[idx].dosingRate=ml/CFG.CALIB_SEC;updateChannel(idx);}}

function cancelChanges(idx){editingChannel=-1;loadStatus();}

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
                if(i===editingChannel)return;
                if(channels[i]){
                    channels[i].events=chData.events||0;
                    channels[i].days=chData.days||0;
                    channels[i].dailyDose=chData.dailyDose||0;
                    channels[i].dosingRate=chData.dosingRate||0.33;
                    channels[i].enabled=chData.enabled!==false;
                    channels[i].name=chData.name||('CH'+i);
                    channels[i].eventsCompleted=chData.eventsCompleted||0;
                    channels[i].eventsFailed=chData.eventsFailed||0;
                    channels[i].state=chData.state||'inactive';
                    channels[i].containerMl=chData.containerMl||1000;
                    channels[i].remainingMl=chData.remainingMl||1000;
                    channels[i].remainingPct=chData.remainingPct||100;
                    channels[i].lowVolume=chData.lowVolume||false;
                    channels[i].daysRemaining=chData.daysRemaining||999;
                    channels[i].totalDosedMl=chData.totalDosedMl||0;
                }
            });
            if(editingChannel===-1)renderChannels();
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
        if(data.success){
            channels[channel].remainingMl=data.remaining_ml;
            channels[channel].remainingPct=100;
            channels[channel].lowVolume=false;
            updateContainerDisplay(channel,{remaining_ml:data.remaining_ml,remaining_pct:100,container_ml:data.container_ml,low_warning:false});
        }
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
    .then(data=>{editingChannel=-1;if(data.success){ch.state='pending';renderChannels();renderOverview();showAlert('Success','Configuration saved. Changes active from tomorrow.','ok');}else{showAlert('Error','Save failed: '+(data.error||'Unknown error'),'err');}})
    .catch(()=>showAlert('Error','Connection error','err'));
}

function toggleChannelEnabled(idx){
    const ch=channels[idx];
    const newEnabled=!ch.enabled;
    fetch('api/dosing-config',{method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({channel:idx,enabled:newEnabled})})
    .then(r=>r.json()).then(data=>{
        if(data.success||data.hasPending){
            ch.enabled=newEnabled;
            // Odśwież kartę kanału
            const card=document.querySelector(`.channel-card[data-ch="${idx}"]`);
            if(card){card.outerHTML=renderChannelCard(ch,idx);}
            bindChannelEvents(idx);
        }
    }).catch(err=>console.error('Toggle enabled error:',err));
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
    if(dosedLabel)dosedLabel.textContent=`dosed ${ch.totalDosedMl.toFixed(1)} ml`;
}

function updateContainerDisplay(idx,data){
    const bar=document.getElementById(`containerBar_${idx}`);
    const remainingLabel=document.getElementById(`remainingLabel_${idx}`);
    if(bar){const pctVal=data.remaining_pct!==undefined?data.remaining_pct:(data.remaining_ml/data.container_ml*100);bar.style.width=`${pctVal}%`;bar.classList.toggle('low',data.low_warning);}
    if(remainingLabel){remainingLabel.textContent=`remaining ${data.remaining_ml.toFixed(0)} ml`;remainingLabel.classList.toggle('low',data.low_warning);}
    if(channels[idx]){channels[idx].containerMl=data.container_ml;channels[idx].remainingMl=data.remaining_ml;channels[idx].remainingPct=data.remaining_pct;channels[idx].lowVolume=data.low_warning;}
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
let alertCallback=null;

function showAlert(title,msg,type,callback){
    document.getElementById('alertIcon').className='modal-icon '+type;
    document.getElementById('alertIcon').innerHTML=MODAL_ICONS[type]||MODAL_ICONS.info;
    document.getElementById('alertTitle').textContent=title;
    document.getElementById('alertText').textContent=msg;
    document.getElementById('alertActions').innerHTML='<button class="btn btn-primary" onclick="closeAlert()">OK</button>';
    alertCallback=callback||null;
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
    document.getElementById('alertModal').classList.remove('show');
    if(confirmed&&alertCallback)alertCallback();
    alertCallback=null;
}
document.addEventListener('DOMContentLoaded',init);
</script>
</body>
</html>
)rawliteral";

String getLoginHTML(){return String(LOGIN_HTML);}
String getDashboardHTML(){return String(DASHBOARD_HTML);}