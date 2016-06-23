# Portaudio_VOIP
Minimalist voice over ip implementation using portaudio

#--Dependencies--
For OSX, should run fine. <br />
For Linux, requires portaudio-19 <br />
To install... <br />
  Use your package manager (apt-get, yum, ...) and install portaudio19-dev

# --Directions--
Make release for use, debug for debugging <br />
Usage: ./exec -h will print usage menu <br />
server: ./exec -r 0 <-m TCP/UDP> <br />
client: ./exec -r 1 -s hostname:port <-m TCP/UDP> <br />
client and server must both run TCP or UDP <br />