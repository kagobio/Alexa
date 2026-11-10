---
name: echo-automation
description: >-
  Automate and personalize YOUR OWN Amazon Echo / Alexa devices. Use this skill
  whenever the user wants to "trastear", customize, control, or automate their
  own Echo — building Alexa Routines, triggering announcements or text-to-speech
  from scripts, integrating Alexa with Home Assistant or Node-RED, driving the
  device through community tools (alexapy, alexa_remote_control.sh,
  alexa-remote2), or developing a custom Alexa Skill with the Alexa Skills Kit
  and a serverless backend. Trigger it on phrases like "automatizar mi Echo",
  "hacer que Alexa diga X", "controlar Alexa por script", "integrar Alexa con
  Home Assistant", "crear una Alexa Skill", or "personalizar mi Alexa", even
  when the user doesn't name a specific tool. This skill is strictly for devices
  and accounts the user owns or is authorized to manage.
---

# Echo / Alexa Automation

Help the user automate and personalize **their own** Amazon Echo devices. There
are four distinct approaches, each suited to a different goal. Pick the one that
matches what the user wants rather than defaulting to code — Routines solve most
everyday needs with no programming at all.

## Scope and safety

This skill covers only devices and Amazon accounts the user owns or is
explicitly authorized to manage. All approaches here authenticate as the
account owner through supported or community-maintained channels — none of them
bypass authentication, exploit vulnerabilities, or access other people's
devices. If a request drifts toward unauthorized access to someone else's
device or account, stop and clarify before continuing.

The unofficial tools (approach 3) log in with the user's own credentials and can
break when Amazon changes its login flow. They are a legitimate, widely used way
to script your own Echo, but set expectations: they are unofficial and may need
re-authentication periodically.

## Choosing an approach

| Goal | Approach | Code needed |
|------|----------|-------------|
| Chain actions on a schedule or trigger ("every morning, read the weather") | **1. Alexa Routines** | None |
| Tie the Echo into a broader smart-home setup, custom TTS, complex logic | **2. Home Assistant / Node-RED** | Config / flows |
| Script the Echo directly: announce, speak, play, control from a server or cron | **3. Community tools** | Shell / Python / JS |
| Build a new voice capability users invoke by name ("Alexa, open …") | **4. Custom Alexa Skill** | Node/Python backend |

When the user is unsure, ask what they want to *happen* (not which tool), then
map it to the table above.

## Approach 1 — Alexa Routines (no code)

The right starting point for most personalization. Routines live in the Alexa
app and chain a trigger (voice phrase, time, alarm, device state, location) to a
sequence of actions (speak, announce, play media, control smart devices, adjust
volume, run in sequence with waits).

Read `references/routines.md` for the full trigger/action catalog, the sequence
and wait mechanics, and common recipes. Guide the user through the app steps; you
can't create a Routine for them programmatically, but you can design the exact
trigger/action sequence and walk them through entering it.

## Approach 2 — Home Assistant / Node-RED

When the Echo is one piece of a larger smart-home setup, or the user wants logic
that Routines can't express (conditionals, loops, external data, custom TTS on
demand), integrate through Home Assistant or Node-RED.

Read `references/home-assistant.md` for: the `alexa_media_player` custom
integration (HACS), exposing entities to Alexa, `notify` TTS and announcements,
and the equivalent Node-RED `alexa-remote2` node flows.

## Approach 3 — Community control tools (scripting)

To drive the Echo directly from a script, server, or cron job — make it speak
arbitrary text, announce, play a station, set volume, query device state —
use a community tool that authenticates with the user's own account:

- **`alexapy`** (Python) — the library behind Home Assistant's integration; good
  for Python automation.
- **`alexa_remote_control.sh`** (Bash) — a single script, great for cron and
  quick CLI control.
- **`alexa-remote2`** (Node.js) — the library behind the Node-RED node.

Read `references/community-tools.md` for install, authentication (including the
cookie/2FA flow), and worked examples (TTS, announce, play, volume, device
list). The skill bundles `scripts/alexa_tts.py` as a ready starting point for
Python-based TTS/announce automation — adapt it rather than writing from scratch.

## Approach 4 — Custom Alexa Skill (ASK)

When the user wants a *new capability* invoked by name ("Alexa, open mi casa"),
build a custom Skill with the Alexa Skills Kit: an interaction model (invocation
name, intents, slots, sample utterances) plus a backend (AWS Lambda or an HTTPS
endpoint) that handles requests.

Read `references/custom-skill.md` for the interaction-model JSON, the
request/response lifecycle, an ASK SDK handler skeleton, and local testing with
the ASK CLI. This is full software development — scaffold the project, set up the
backend, and iterate on the interaction model.

## Working pattern

1. Identify the goal and map it to an approach using the table above.
2. Read the matching reference file before writing config or code.
3. For scripting (approach 3/4), confirm the account is the user's own and walk
   through authentication carefully — it's the step that most often trips people
   up.
4. Prefer the simplest approach that meets the need; don't reach for Lambda when
   a Routine would do.
