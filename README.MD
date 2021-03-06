*This is a multi-threaded TCP-IP shell and file server capable of accepting telnet connections.  The program runs in the background as a Linux daemon, using the project root as the working directory. The log file is in the program's root.*

---
Standardize return
---

> Status Code Message

All server command produces a standardize response message as the above.

---

Shell server commands:
---

>[any available Linux command with args]

Run this command on the server.

>CPRINT

Return the command line printing of the lastest command.

---
File server commands:
---
>FOPEN filename 

Open a file, return an identifier in "Code". All following commands must be using identifiers that were previously given by this command.

>FSEEK identifier offset 

To advance the reading/writing pointer on a file the "offset" amount, relative to the current pointer position.

>FREAD identifier length 

To read up to length bytes of data from a file. Return the red content in "Message."

>FWRITE identifier content

To write to a file with the content.

>FCLOSE identifier

To close a file that was previously opened by FOPEN.

---
File Access Protection
---

**To a particular file:**

Simultaneous reading is allowed.

No opening can occur when the file is being written.


No writing can occur when someone else is doing it.


No Writing can occur when others are reading.


No Reading can occur when someone is writing.

---
Command-line arguments
---
>-f port

Specify a file server port

>-s port

Specify a shell server port

>-d

Do not detach. Run the server in the foreground.

>-D

Debug file access, more logging, delayed read/write action.



