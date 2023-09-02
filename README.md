<h1>Snip</h1>
<p>tool to make screenshots (X11 session only)</p>
<h3>usage</h3>
<p>snip <path>. default path is "~/snip.png"</p>
<h3>tip</h3>
<h5>to improve your experience you can create script file</h5>
<p>#!/bin/bash
snip "/home/$USER/snip.png"
cat /home/$USER/snip.png | xclip -selection clipboard -target image/png -i
</p>
<p>I placed it into /usr/bin folder so I can exec it by dmenu or create hotkey</p>
