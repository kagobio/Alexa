# Alexa Routines

Routines chain a **trigger** to a sequence of **actions**. They are created in
the Alexa app (More → Routines → +) or at alexa.amazon.com. There is no public
API to create them programmatically, so your job is to design the exact
trigger/action sequence and walk the user through entering it.

## Triggers ("When this happens")

- **Voice** — a custom phrase, e.g. "Alexa, buenos días". Keep it short and
  distinct so it isn't confused with built-ins.
- **Schedule** — a specific time, optionally repeating on chosen days; or
  sunrise/sunset (± offset) if a location is set.
- **Alarm dismissed** — fires when the user dismisses an alarm (great for a
  morning routine).
- **Smart Home / device state** — a sensor or switch changes (motion, contact,
  a plug turning on), if the device is linked to Alexa.
- **Echo Button / location** (arrive/leave, via the phone app).
- **Guard** (away/home) where available.

## Actions ("Alexa will")

- **Alexa Says** — speak custom text, tell a joke, read weather/news/traffic,
  flash briefing.
- **Announcement** — broadcast text to all (or selected) Echo devices.
- **Messaging** — send a notification to the app.
- **Smart Home** — turn devices on/off, set brightness/color, scenes.
- **Music & Media** — play a station/playlist from a chosen provider on a chosen
  device, for a set duration.
- **Volume** — set device volume (set it *before* a TTS/announcement action so
  the message plays at a predictable level).
- **Wait** — pause N seconds/minutes between actions to sequence things.
- **Device Settings** — Do Not Disturb, adjust brightness, etc.

Actions run top-to-bottom in order. Target a specific device per action where the
option is offered, or "the device you speak to".

## Recipes

**Morning briefing (voice or alarm-dismissed trigger):**
1. Set volume → 4
2. Alexa Says → "Buenos días. Aquí está tu resumen."
3. Weather
4. Traffic (if commute is configured)
5. Play → news flash briefing

**Good night (voice "Alexa, buenas noches"):**
1. Smart Home → turn off "Luces salón"
2. Alexa Says → "Que descanses"
3. Device Settings → Do Not Disturb on
4. Music → rain sounds for 30 minutes

**Arriving home (location trigger, phone app):**
1. Smart Home → turn on entry lights
2. Announcement → "Bienvenido a casa"

## Tips

- Volume-before-speech is the single most useful ordering trick; otherwise a
  night-time announcement can blast at daytime volume.
- Use **Wait** to let one action (e.g. lights coming up) finish before the next.
- Announcements go to every Echo unless you scope the device; use "Alexa Says"
  on a single device for a local-only message.
- A Routine can be triggered by another Routine's announcement only indirectly;
  for real chaining/conditionals, move to Home Assistant/Node-RED.
