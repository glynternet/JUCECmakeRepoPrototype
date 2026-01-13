In the TODOs secion, each ### corresponds to a single TODO.
The TODOs are in an arbitrary order.
The title of the TODO is the title given by the ### line.
For a given TODO, a plan should be written to todos/plans/ as a markdown file.
Plans should:
- include high-level design
- include low-level example implementations for how the work would be done
- new classes/interfaces/types should have their APIs documented as well as interactions with other system actors
- include pros and cons from a performance perspective
- include any questions that were asked and answered during requirements elicitation
When commiting the completion of a plan, include the plan document content at the bottom of the commit, then delete the corresponding plan document and TODO item in this file.

## TODOs

### Rate-limit "Sender not connected" log line
When the OSC sender is not connected, that log line gets logged on every attempted message send. We should limit that log line to one every 30s.
