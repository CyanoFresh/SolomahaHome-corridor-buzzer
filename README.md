SolomahaHome Corridor Buzzer
===

Unlock intercom (buzzer) and detect call.

## Install

Clone this and check `./.trabis.yml`

## Used topics

- `buzzer/{id}/unlocked` - `true` or `false` is sent if it was unlocked
- `buzzer/{id}/ringing` - `true` or `false` when call detected
- `buzzer/{id}/unlock` - Send anything here to start unlocking operation