# Home Assistant & Node-RED integration

Use this when the Echo is part of a larger smart-home setup, or when you need
logic Routines can't express: conditionals, loops, external data, or custom TTS
fired on demand from any automation.

## Home Assistant — alexa_media_player (HACS)

`alexa_media_player` is a custom integration (install via HACS) built on
`alexapy`. It logs in with the account owner's credentials and exposes each Echo
as a `media_player` plus services for TTS and announcements.

Setup:

1. Install HACS, then add **Alexa Media Player** from HACS → Integrations.
2. Settings → Devices & Services → Add Integration → Alexa Media Player.
3. Enter the Amazon email/password and complete 2FA in the prompt. Choose the
   correct Amazon domain for the marketplace.

Each Echo appears as `media_player.echo_salon`, etc.

### TTS and announcements

```yaml
# Speak on one device
service: notify.alexa_media_echo_salon
data:
  message: "La lavadora ha terminado"
  data:
    type: tts            # tts | announce | push

# Announce to several
service: notify.alexa_media
data:
  message: "Hora de cenar"
  data:
    type: announce
  target:
    - media_player.echo_salon
    - media_player.echo_cocina
```

### Example automation (sensor → spoken alert)

```yaml
automation:
  - alias: "Avisar puerta abierta de noche"
    trigger:
      - platform: state
        entity_id: binary_sensor.puerta_entrada
        to: "on"
    condition:
      - condition: time
        after: "23:00:00"
        before: "06:00:00"
    action:
      - service: notify.alexa_media_echo_dormitorio
        data:
          message: "La puerta de entrada se ha abierto"
          data:
            type: announce
```

You also get `media_player` controls (play/pause/volume/source) and sensors for
last-called device, next timer/alarm, and DND — build any logic on top of those.

### Exposing HA entities to voice

To say "Alexa, enciende el ventilador" for HA-only devices, expose them through
**Alexa (Amazon) Smart Home** — either Nabu Casa Cloud (easiest) or a self-hosted
Smart Home Skill. That's separate from `alexa_media_player`, which is about
controlling the Echo *from* HA rather than controlling HA *from* Alexa.

## Node-RED — alexa-remote2 node

Install `node-red-contrib-alexa-remote2`. Add an **Alexa Remote Account** config
node and authenticate with the proxy method (open the proxy URL, log in, 2FA
once). Then drive devices from flows:

- **alexa-remote-routine** node, mode *speak* → TTS arbitrary text
- mode *announcement* → broadcast
- mode *tunein* / *playmusic* → start media
- **alexa-remote-event** node → react to device activity as flow triggers

Typical flow: `inject`/event → `function` (build message text) →
`alexa-remote-routine (speak)` → done. This gives you full JS logic around the
Echo without running a separate script host.

## Choosing HA vs Node-RED

- Already run **Home Assistant** → use `alexa_media_player`; it's native to HA
  automations and dashboards.
- Want **visual flow logic** or already run **Node-RED** → use `alexa-remote2`.
- Both authenticate the same way (owner credentials + 2FA) and can coexist, but
  sharing one cookie/session across both can cause re-login churn — prefer one as
  the primary controller.
