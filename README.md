# C++ MySQL Library Management System (Console Version) 
This is a C++ console application that provides a complete library management system.
All interactions (user login, admin controls, book issuing) are handled through the command-line interfac

## Features

### User Features
User Login & Sign-up: Securely (in concept) log in or create a new user account.
View Profile: See your user details.
View All Books: Browse the entire library catalog.
Issue Book: Borrow a book from the library.
Deposit Book: Return a book to the library.
View My Books: See a list of all books you currently have on loan.

### Administrator Features
(Protected by a hard-coded password: AAYUSH)
#### User Management:
Show all registered users.
Search for a specific user by ID.
Add a new user.
Delete a user.
Modify a user's details.

#### Book Management:
Show all books in the catalog.
Add a new book (or add copies if it already exists).
Delete a book from the catalog.
Modify a book's details.

#### Loan Management:
View all active loans from all users.

## How to Set Up and Run
1. Prerequisites
A running MySQL Server (like MariaDB).
A C++ compiler and environment, such as MSYS2 UCRT64.
The MySQL C Connector library.

2. Database Setup
Open your MySQL client (like MySQL Workbench).
Create a new database. The code defaults to Library, but you can change this.
```
CREATE DATABASE IF NOT EXISTS Library;
USE Library;
```
Run the setup.sql script to create the required users, books, and loans tables.

3. Install Dependencies (MSYS2)
Open your MSYS2 UCRT64 terminal and install the C++ compiler and the MariaDB C connector:
```
pacman -S mingw-w64-ucrt-x86_64-gcc-g++ mingw-w64-ucrt-x86_64-libmariadbclient
```

4. Configure Code
Open mysql_library.cpp and edit the configuration block at the top of the file to match your MySQL server details:
```
// Change these to match your server details!
const char* HOST = "127.0.0.1";
const char* USER = "root";
const char* PASS = "your_password"; // <-- IMPORTANT: CHANGE THIS
const char* DB = "test_db";       // <-- Change this if you used a different DB name
```

5. Compile
In your MSYS2 UCRT64 terminal, navigate to your project folder and run the following command to compile the program:
```
g++ mysql_library.cpp -o mysql_library.exe -std=c++17 -I/c/msys64/ucrt64/include/mariadb -lmariadbclient -lcurl -lws2_32 -lz -lzstd -lsecur32 -lcrypt32 -lbcrypt -lshlwapi
```

6. Run

Run the compiled executable from the same terminal:
```
./mysql_library.exe
```
You should see "Successfully connected to MySQL database" and the main menu will appear.

## Security Warning: SQL Injection
This code builds SQL queries by concatenating strings:
e.g., "SELECT * FROM users WHERE name = '" + name + "'"
This is DANGEROUS in a real-world application because it is vulnerable to SQL Injection attacks. A user could enter ' OR '1'='1 as their name and bypass security.
The correct, secure method is to use Prepared Statements, which treat user input as data, not as part of the SQL command. This project uses string concatenation for simplicity to demonstrate the conversion from file-based logic to database logic.

