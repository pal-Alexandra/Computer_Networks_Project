#   MyFileTransferProtocol

This repository contains the project developed for the Computer Networks course at the Faculty of Informatics, Alexandru Ioan Cuza University, titled MyFileTransferProtocol.

## Overview
The MyFileTransferProtocol project is built upon the TCP protocol and it provides a set of functionalities for managing files and directories.

## Use cases

The server program relies on two essential files:
* whitelist.txt: This file should contain lines with the pattern <username>. These usernames are determined by the network administrator, and only users listed here are allowed to execute specific commands.
* login.txt: This file should contain lines with the pattern <username> <password>. It enumerates the authorized users of the application, associating each user with a corresponding password.

## Server Operations
The server application operates within a root directory (server_dir in the current folder) and supports the following operations:
* exit: Closes the communication between the server and clients.
* login: Initiates user login.
* logout: Logs out the currently logged-in user.
* my_mkdir: Creates a new folder in the current directory.
* my_rm: Deletes a folder in the current directory.
* my_chdir: Changes the current working directory to a valid directory.
* my_delete: Deletes a file.
* my_rename: Renames a file.
* my_mv: Moves a file.
* my_download: Downloads a file.
* help: Lists the valid commands and their descriptions.
* list: Lists the contents of the current directory.
* location: Displays the current working directory path.

If a user not listed in the whitelist.txt file attempts to connect, they will only have access to the following commands: exit and help.




