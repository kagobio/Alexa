# Community control tools

These tools let you script **your own** Echo directly. They authenticate with the
account owner's credentials (the same login the Alexa app uses) and call the
same private endpoints the app does. They are unofficial: Amazon can change the
login flow, so expect occasional re-authentication. Never use them against an
account you don't control.

Three mature options, pick by language:

| Tool | Language | Best for |
|------|----------|----------|
| `alexapy` | Python | Python automation; backs Home Assistant's integration |
| `alexa_remote_control.sh` | Bash | cron jobs, quick CLI, one-file deploys |
| `alexa-remote2` | Node.js | JS apps; backs the Node-RED `alexa-remote2` node |

## Authentication (the part that trips people up)

Amazon requires a real login and, almost always, 2FA. All three tools solve this
by capturing a login **cookie** once and reusing it. Two common paths:

- **Proxy login** (recommended): the tool starts a local web server (e.g.
  `http://localhost:3456`); you open it in a browser, log in to Amazon normally
  (including 2FA), and it captures the cookie. `alexa_remote_control.sh` and
  `alexa-remote2` both support this.
- **App-password / OTP secret**: if the account has an authenticator-app 2FA
  secret, some tools can generate the OTP automatically for headless re-login.

Pick the right Amazon domain (`amazon.com`, `amazon.es`, `amazon.co.uk`, …) and
the matching Alexa host — a mismatch is the most common cause of login failures.

Store the cookie file with tight permissions (`chmod 600`); it is as sensitive as
a password.

## alexa_remote_control.sh (Bash)

```bash
# one-time: configure domain + run the proxy login
./alexa_remote_control.sh -a                 # list devices
./alexa_remote_control.sh -d "Echo Salón" -e speak:"Hola desde el script"
./alexa_remote_control.sh -d "Echo Salón" -e announcement:"La cena está lista"
./alexa_remote_control.sh -d "Echo Salón" -e vol:40
./alexa_remote_control.sh -d "Echo Salón" -e textcommand:"pon las noticias"
```

Great in cron:

```cron
0 8 * * 1-5 /opt/alexa/alexa_remote_control.sh -d "Echo Dormitorio" -e speak:"Son las ocho, hora de levantarse"
```

## alexapy (Python)

```python
import asyncio
from alexapy import AlexaLogin, AlexaAPI

async def main():
    login = AlexaLogin("amazon.es", "EMAIL", "PASSWORD", outputpath="/home/me/.alexa")
    await login.login()               # follow the proxy/2FA prompt once
    devices = await AlexaAPI.get_devices(login)
    echo = next(d for d in devices if d["accountName"] == "Echo Salón")
    api = AlexaAPI(echo, login)
    await api.send_tts("Hola desde Python")
    await api.send_announcement("La cena está lista")
    await api.set_volume(0.4)

asyncio.run(main())
```

See `scripts/alexa_tts.py` in this skill for a runnable starting point; adapt the
device name and message rather than writing a new client from scratch.

## alexa-remote2 (Node.js)

```js
const Alexa = require('alexa-remote2');
const alexa = new Alexa();
alexa.init({ cookie: process.env.ALEXA_COOKIE, amazonPage: 'amazon.es',
             proxyOnly: true, proxyOwnIp: 'localhost', proxyPort: 3456 },
  (err) => {
    if (err) return console.error(err);
    alexa.sendSequenceCommand('Echo Salón', 'speak', 'Hola desde Node', () => {});
  });
```

## What you can do once authenticated

- `speak` / TTS — say arbitrary text on a device
- `announcement` — broadcast to one or many devices
- `vol` — set volume
- `textcommand` — send any phrase as if spoken ("pon música", "apaga las luces")
- play/pause/next, play a TuneIn station or playlist
- read device state, last-heard, notifications, shopping/to-do lists
- set/cancel timers and reminders

## Troubleshooting

- **Login loops / captcha** → use the proxy login in a real browser; headless
  logins hit captcha.
- **Works then stops after days** → cookie expired; re-run the proxy login or
  configure the OTP secret for auto-refresh.
- **Device not found** → names are exact and locale-specific; list devices first.
- **Wrong domain** → match `amazon.<tld>` to the account's marketplace.
