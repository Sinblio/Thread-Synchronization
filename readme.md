#Encrypt352

Created by Benjamin Schroeder

##main.c

Acts as a interface allowing for a user to configure and run the encryption process laid out within the the encrypt-module.h and encrypt-module.c classes

##encrypt-module.c & encrypt_module.h

Creates an implementation allowing for encryption to be processed between files across multiple threads, with a randomly changing key. Also provides tracking statistics to the user who is running the program. Inorder to fulfil this a list is implemented with a capped length inorder to respect buffer limitations put in place by the user.
