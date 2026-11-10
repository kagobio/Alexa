# Custom Alexa Skill (Alexa Skills Kit)

Build a new capability the user invokes by name: "Alexa, abre mi casa". A custom
Skill has two halves:

1. **Interaction model** — the invocation name, intents, slots, and sample
   utterances (JSON, edited in the Alexa Developer Console or via the ASK CLI).
2. **Backend** — code that receives JSON requests and returns JSON responses.
   Host it on **AWS Lambda** (easiest; Amazon hosts an option too) or any HTTPS
   endpoint with a valid TLS cert.

Account needed: a free **Amazon Developer** account (developer.amazon.com), plus
AWS if using Lambda.

## Interaction model (skeleton)

```json
{
  "interactionModel": {
    "languageModel": {
      "invocationName": "mi casa",
      "intents": [
        { "name": "AMAZON.HelpIntent", "samples": [] },
        { "name": "AMAZON.StopIntent", "samples": [] },
        { "name": "AMAZON.CancelIntent", "samples": [] },
        {
          "name": "EstadoIntent",
          "slots": [
            { "name": "habitacion", "type": "AMAZON.Room" }
          ],
          "samples": [
            "cómo está {habitacion}",
            "dime el estado de {habitacion}",
            "qué pasa en {habitacion}"
          ]
        }
      ],
      "types": []
    }
  }
}
```

Invocation name rules: lowercase, two+ words recommended, no wake words, no
branded terms you don't own. Each custom intent needs several natural sample
utterances — the more varied, the better recognition.

## Backend (ASK SDK v2, Node.js)

```js
const Alexa = require('ask-sdk-core');

const LaunchHandler = {
  canHandle(h) {
    return Alexa.getRequestType(h.requestEnvelope) === 'LaunchRequest';
  },
  handle(h) {
    return h.responseBuilder
      .speak('Hola, ¿qué quieres saber?')
      .reprompt('¿Qué habitación?')
      .getResponse();
  },
};

const EstadoHandler = {
  canHandle(h) {
    return Alexa.getRequestType(h.requestEnvelope) === 'IntentRequest'
        && Alexa.getIntentName(h.requestEnvelope) === 'EstadoIntent';
  },
  handle(h) {
    const room = Alexa.getSlotValue(h.requestEnvelope, 'habitacion') || 'la casa';
    // ...look up real state here...
    return h.responseBuilder.speak(`Todo bien en ${room}.`).getResponse();
  },
};

exports.handler = Alexa.SkillBuilders.custom()
  .addRequestHandlers(LaunchHandler, EstadoHandler)
  .lambda();
```

Python (ask-sdk-core) follows the same handler shape if the user prefers Python.

## Request/response lifecycle

1. User speaks → Alexa matches an intent from the interaction model.
2. Alexa POSTs a JSON request (type `LaunchRequest`, `IntentRequest`,
   `SessionEndedRequest`) to your backend.
3. Your handler returns JSON with `outputSpeech`, optional `reprompt`, `card`,
   and whether to keep the session open (`shouldEndSession`).
4. Keep a turn short; for multi-turn, leave the session open and set a reprompt.

Always include `AMAZON.HelpIntent`, `AMAZON.StopIntent`, and
`AMAZON.CancelIntent` handlers — certification requires them.

## Workflow with the ASK CLI

```bash
npm install -g ask-cli
ask configure                 # link developer + AWS accounts
ask new                       # scaffold model + Lambda
ask deploy                    # push model and backend
ask dialog                    # simulate conversations locally
```

For personal use you don't need to publish/certify — a skill stays in
**Development** mode and works on the Echoes linked to the same Amazon account.
Only submit for certification if you intend to share it publicly.

## When NOT to build a custom Skill

If the goal is "make Alexa say/do X on a trigger", a Routine (approach 1) or a
script (approach 3) is far less work. Reach for a custom Skill only when you need
a named, conversational capability with its own backend logic.
