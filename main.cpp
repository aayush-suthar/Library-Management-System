#include <iostream>
#include <string>
#include <sstream>      
#include <stdexcept>  
#include <fstream>  
#include <random>
#include <ctime>
#include <mysql.h>      

using namespace std;

// ##################################################################
// ################ !! IMPORTANT CONFIGURATION !! ##################
// ##################################################################

const char* HOST = "127.0.0.1";
const char* USER = "root"; 
const char* DB = "Library";

// Global database connection object
MYSQL* conn;

// Random number generator for IDs
random_device rd;
mt19937 gen(std::time(nullptr));
uniform_int_distribution<int> dist(10000, 99999);


/*
====================================================================
 CRITICAL: Security Warning - SQL Injection
====================================================================
This code builds SQL queries by concatenating strings:
e.g., "SELECT * FROM users WHERE name = '" + name + "'"

This is DANGEROUS in a real-world application because it is
vulnerable to "SQL Injection" attacks. A user could enter
`' OR '1'='1` as their name and bypass security.

The correct, secure method is to use "Prepared Statements,"
which treat user input as data, not as part of the SQL command.

I have used the simpler string method to keep the code clear

Further improvement will be made with time
====================================================================
*/


// ############################################################
// ################ DATABASE HELPER FUNCTIONS #################
// ############################################################

/**
 * @brief Executes a MySQL query and checks for errors.
 * Throws a runtime_error if the query fails.
 *
 * @param query The SQL query string to execute.
 */

void exec_query(const string& query){
    if(mysql_query(conn, query.c_str())){
        // If the query fails, throw an exception
        string error = "SQL ERROR: " + string(mysql_error(conn));
        throw runtime_error(error);
    }
}

/**
 * @brief Checks if a SELECT query returns any rows.
 *
 * @param query The SELECT query string.
 * @return true if at least one row is returned, false otherwise.
 */
bool check_exists(const string& query){
    if(mysql_query(conn, query.c_str())){
        string error = "SQL ERROR: " + string(mysql_error(conn));
        throw runtime_error(error);
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if(result == NULL){
        throw runtime_error("mysql_store_result failed");
    }

    bool exists =(mysql_num_rows(result)> 0);
    mysql_free_result(result);
    return exists;
}


// ############################################################
// ###################### USER CLASS ##########################
// ############################################################

class User{
    // These properties are now private and set on login
    string user_id;
    string name;
    string password;
    string phone;

public:
    // Default constructor
    User(){}

    // Constructor to initialize a logged-in user
    User(string user_id, string name, string password, string phone){
        this->user_id = user_id;
        this->name = name;
        this->password = password;
        this->phone = phone;
    }

    // Static function for adding a new user(replaces old Add_User)
    static bool Add_User(bool from_admin = false);

    // Static function to log in a user
    static void Login();

    // Member functions for a logged-in user
    void User_Main_Menu();
    void Show_User();
    void My_Books();
    void Issue_Book();
    void Deposit_Book();
};


// ############################################################
// ################## GENERAL DB FUNCTIONS ####################
// ############################################################

void Display_All_Book(){
    system("cls");
    cout<<"\t\t\t\t\t\t--- ALL BOOKS IN LIBRARY ---\n\n";

    string query = "SELECT * FROM books";
    if(mysql_query(conn, query.c_str())){
        throw runtime_error(mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if(result == NULL){
        throw runtime_error("Failed to store query result");
    }

    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        cout<<"\t\t\t\t\t\tID               : "<<row[0]<<"\n";
        cout<<"\t\t\t\t\t\tNAME             : "<<row[1]<<"\n";
        cout<<"\t\t\t\t\t\tAUTHOR           : "<<row[2]<<"\n";
        cout<<"\t\t\t\t\t\tEDITION          : "<<row[3]<<"\n";
        cout<<"\t\t\t\t\t\tPUBLISH YEAR     : "<<row[4]<<"\n";
        cout<<"\t\t\t\t\t\tCOPIES AVAILABLE : "<<row[5]<<"\n";
        cout<<"\t\t\t\t\t\tISBN             : "<<row[6]<<"\n\n";
    }

    mysql_free_result(result);
    cout<<"Enter 0 to return: ";
    string x;
    cin>>x;
}

void User::Issue_Book(){
    system("cls");
    cout<<"Enter -1 to return in any below field\n";
    string name, author, isbn;
    int copies, edition;

    cout<<"\t\t\t\t\t\t--- ISSUE A BOOK ---\n";
    cout<<"\t\t\t\t\t\tNAME      : "; cin>>name; if(name == "-1")return;
    cout<<"\t\t\t\t\t\tAUTHOR    : "; cin>>author; if(author == "-1")return;
    cout<<"\t\t\t\t\t\tEDITION   : "; cin>>edition; if(edition == -1)return;
    cout<<"\t\t\t\t\t\tISBN      : "; cin>>isbn; if(isbn == "-1")return;
    cout<<"\t\t\t\t\t\tCOPIES    : "; cin>>copies; if(copies == -1)return;

    // Find the book
    stringstream ss_find;
    ss_find<<"SELECT book_id, copies FROM books WHERE isbn = '"<<isbn 
           <<"' AND name = '"<<name<<"' AND author = '"<<author 
           <<"' AND edition = "<<edition;

    if(mysql_query(conn, ss_find.str().c_str())){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* res_find = mysql_store_result(conn);
    if(mysql_num_rows(res_find)== 0){
        cout<<"Book not found. Enter any key: ";
        mysql_free_result(res_find);
        string x; cin>>x;
        return;
    }

    MYSQL_ROW row_find = mysql_fetch_row(res_find);
    string book_id = row_find[0];
    int available_copies = stoi(row_find[1]);
    mysql_free_result(res_find);

    if(copies > available_copies){
        cout<<"ONLY "<<available_copies<<" COPIES ARE AVAILABLE. Enter any key: ";
        string x; cin>>x;
        return;
    }

    // 1. Update book count
    stringstream ss_update_book;
    ss_update_book<<"UPDATE books SET copies = copies - "<<copies 
                  <<" WHERE book_id = '"<<book_id<<"'";
    exec_query(ss_update_book.str());

    // 2. Add to loans
    // Check if a loan for this book/user already exists
    stringstream ss_find_loan;
    ss_find_loan<<"SELECT loan_id FROM loans WHERE book_id = '"<<book_id 
                <<"' AND user_id = '"<<this->user_id<<"'";

    if(check_exists(ss_find_loan.str())){
        // Loan exists, just update the copy count
        stringstream ss_update_loan;
        ss_update_loan<<"UPDATE loans SET copies = copies + "<<copies
                      <<" WHERE book_id = '"<<book_id 
                      <<"' AND user_id = '"<<this->user_id<<"'";
        exec_query(ss_update_loan.str());
    }else{
        // No loan exists, create a new one
        string loan_id = "L" + to_string(dist(gen));
        stringstream ss_insert_loan;
        ss_insert_loan<<"INSERT INTO loans(loan_id, book_id, user_id, copies, issue_date)"
                      <<"VALUES('"<<loan_id<<"', '"<<book_id<<"', '" 
                      <<this->user_id<<"', "<<copies<<", NOW())";
        exec_query(ss_insert_loan.str());
    }

    cout<<"Book Issued Successfully. Enter any key: ";
    string x; cin>>x;
}

void User::Deposit_Book(){
    system("cls");
    cout<<"Enter -1 to return in any below field\n";
    string name, author, isbn;
    int copies_to_deposit, edition;

    cout<<"\t\t\t\t\t\t--- DEPOSIT A BOOK ---\n";
    cout<<"\t\t\t\t\t\tNAME      : "; cin>>name; if(name == "-1")return;
    cout<<"\t\t\t\t\t\tAUTHOR    : "; cin>>author; if(author == "-1")return;
    cout<<"\t\t\t\t\t\tEDITION   : "; cin>>edition; if(edition == -1)return;
    cout<<"\t\t\t\t\t\tISBN      : "; cin>>isbn; if(isbn == "-1")return;
    cout<<"\t\t\t\t\t\tCOPIES    : "; cin>>copies_to_deposit; if(copies_to_deposit == -1)return;

    // Find the book to get its ID
    stringstream ss_find_book;
    ss_find_book<<"SELECT book_id FROM books WHERE isbn = '"<<isbn
       <<"' AND name = '"<<name<<"' AND author = '"<<author
       <<"' AND edition = "<<edition;
    
    if(mysql_query(conn, ss_find_book.str().c_str())){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* res_book = mysql_store_result(conn);
    if(mysql_num_rows(res_book)== 0){
        cout<<"Book not found. Enter any key: ";
        mysql_free_result(res_book);
        string x; cin>>x;
        return;
    }
    string book_id = mysql_fetch_row(res_book)[0];
    mysql_free_result(res_book);

    // Find the loan
    stringstream ss_find_loan;
    ss_find_loan<<"SELECT copies FROM loans WHERE book_id = '"<<book_id
                <<"' AND user_id = '"<<this->user_id<<"'";
    
    if(mysql_query(conn, ss_find_loan.str().c_str())){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* res_loan = mysql_store_result(conn);
    if(mysql_num_rows(res_loan)== 0){
        cout<<"You have not issued this book. Enter any key: ";
        mysql_free_result(res_loan);
        string x; cin>>x;
        return;
    }
    
    int issued_copies = stoi(mysql_fetch_row(res_loan)[0]);
    mysql_free_result(res_loan);

    if(copies_to_deposit > issued_copies){
        cout<<"You only have "<<issued_copies<<" copies. You cannot deposit "<<copies_to_deposit;
        string x; cin>>x;
        return;
    }

    // 1. Update book count
    stringstream ss_update_book;
    ss_update_book<<"UPDATE books SET copies = copies + "<<copies_to_deposit
                  <<" WHERE book_id = '"<<book_id<<"'";
    exec_query(ss_update_book.str());

    // 2. Update loan count
    if(copies_to_deposit == issued_copies){
        // All copies returned, delete loan record
        stringstream ss_delete_loan;
        ss_delete_loan<<"DELETE FROM loans WHERE book_id = '"<<book_id
                      <<"' AND user_id = '"<<this->user_id<<"'";
        exec_query(ss_delete_loan.str());
    }else{
        // Partially returned, update loan count
        stringstream ss_update_loan;
        ss_update_loan<<"UPDATE loans SET copies = copies - "<<copies_to_deposit
                      <<" WHERE book_id = '"<<book_id
                      <<"' AND user_id = '"<<this->user_id<<"'";
        exec_query(ss_update_loan.str());
    }
    
    cout<<"Deposit Successful. Enter any key: ";
    string x; cin>>x;
}

void User::My_Books(){
    system("cls");
    cout<<"\t\t\t\t\t\t--- MY ISSUED BOOKS ---\n\n";

    stringstream ss;
    ss<<"SELECT b.name, b.author, b.isbn, l.copies, l.issue_date "
      <<"FROM books b JOIN loans l ON b.book_id = l.book_id "
      <<"WHERE l.user_id = '"<<this->user_id<<"'";

    if(mysql_query(conn, ss.str().c_str())){
        throw runtime_error(mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if(mysql_num_rows(result)== 0){
        cout<<"You have no books issued.\n";
    }

    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        cout<<"\t\t\t\t\t\tNAME        : "<<row[0]<<"\n";
        cout<<"\t\t\t\t\t\tAUTHOR      : "<<row[1]<<"\n";
        cout<<"\t\t\t\t\t\tISBN        : "<<row[2]<<"\n";
        cout<<"\t\t\t\t\t\tCOPIES HELD : "<<row[3]<<"\n";
        cout<<"\t\t\t\t\t\tISSUE DATE  : "<<row[4]<<"\n\n";
    }

    mysql_free_result(result);
    cout<<"Enter 0 to return: ";
    string x; cin>>x;
}

void User::Show_User(){
    system("cls");
    cout<<"\t\t\t\t\t\t--- MY PROFILE ---\n";
    cout<<"\t\t\t\t\t\tID       : "<<user_id<<endl;
    cout<<"\t\t\t\t\t\tNAME     : "<<name<<endl;
    cout<<"\t\t\t\t\t\tPASSWORD : "<<password<<endl;
    cout<<"\t\t\t\t\t\tContact  : "<<phone<<"\n\n";
    cout<<"\t\t\t\t\t\tEnter 0 to return ";
    int x; cin>>x;
}

void User::User_Main_Menu(){
    system("cls");
    cout<<"\t\t\t\t\t\t"<<name<<" Main Menu\n"<<endl;
    cout<<"\t\t\t\t\t\t1. View Profile\n";
    cout<<"\t\t\t\t\t\t2. View MyBooks\n";
    cout<<"\t\t\t\t\t\t3. Issue Book\n";
    cout<<"\t\t\t\t\t\t4. Deposit Book\n";
    cout<<"\t\t\t\t\t\t5. Show All Books\n";
    cout<<"\t\t\t\t\t\tChoose from [1-5]\n";
    cout<<"\t\t\t\t\t\tEnter 0 to return"<<endl;
    int opt;
    cin>>opt;

    switch(opt){
    case 1:
        Show_User();
        User_Main_Menu();
        break;
    case 2:
        try{ My_Books(); }
        catch(const exception& e){ cout<<e.what()<<"\nPress key to cont."; string x; cin>>x; }
        User_Main_Menu();
        break;
    case 3:
        try{ Issue_Book(); }
        catch(const exception& e){ cout<<e.what()<<"\nPress key to cont."; string x; cin>>x; }
        User_Main_Menu();
        break;
    case 4:
        try{ Deposit_Book(); }
        catch(const exception& e){ cout<<e.what()<<"\nPress key to cont."; string x; cin>>x; }
        User_Main_Menu();
        break;
    case 5:
        try{ Display_All_Book(); }
        catch(const exception& e){ cout<<e.what()<<"\nPress key to cont."; string x; cin>>x; }
        User_Main_Menu();
        break;
    case 0:
        break;
    default:
        User_Main_Menu();
    }
}


// ############################################################
// ################# ADMINISTRATION FUNCTIONS #################
// ############################################################

void Admin_Add_Book(){
    system("cls");
    cout<<"Enter -1 to return in any below field\n";
    string name, author, isbn, publish_year;
    int edition, copies;

    cout<<"\t\t\t\t\t\t--- ADD A NEW BOOK ---\n";
    cout<<"\t\t\t\t\t\tEnter Book Name    : "; cin>>name; if(name == "-1")return;
    cout<<"\t\t\t\t\t\tEnter Author Name  : "; cin>>author; if(author == "-1")return;
    cout<<"\t\t\t\t\t\tISBN Code          : "; cin>>isbn; if(isbn == "-1")return;
    cout<<"\t\t\t\t\t\tPublish Year       : "; cin>>publish_year; if(publish_year == "-1")return;
    cout<<"\t\t\t\t\t\tEdition(number)   : "; cin>>edition; if(edition == -1)return;
    cout<<"\t\t\t\t\t\tcopies             : "; cin>>copies; if(copies == -1)return;

    // Check if book(by ISBN)already exists
    stringstream ss_check;
    ss_check<<"SELECT copies FROM books WHERE isbn = '"<<isbn<<"'";
    
    if(mysql_query(conn, ss_check.str().c_str())){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* result = mysql_store_result(conn);
    
    if(mysql_num_rows(result)> 0){
        // Book exists, just add copies
        mysql_free_result(result);
        stringstream ss_update;
        ss_update<<"UPDATE books SET copies = copies + "<<copies 
                 <<" WHERE isbn = '"<<isbn<<"'";
        exec_query(ss_update.str());
        cout<<"Book with ISBN "<<isbn<<" already exists. Added "<<copies<<" copies.\n";
    }else{
        // Book is new, insert it
        mysql_free_result(result);
        string id = "B" + to_string(dist(gen));
        stringstream ss_insert;
        ss_insert<<"INSERT INTO books(book_id, name, author, edition, publish_year, copies, isbn)"
                 <<"VALUES('"<<id<<"', '"<<name<<"', '"<<author<<"', " 
                 <<edition<<", '"<<publish_year<<"', "<<copies<<", '"<<isbn<<"')";
        exec_query(ss_insert.str());
        cout<<"New book added successfully.\n";
    }
    cout<<"Enter any key to continue: ";
    string x; cin>>x;
}

void Modify_Book(){
    system("cls");
    string id;
    cout<<"Enter -1 to return\n";
    cout<<"\t\t\t\t\t\tEnter Book ID to Modify: "; cin>>id;
    if(id == "-1")return;

    // Check if book exists
    if(!check_exists("SELECT * FROM books WHERE book_id = '" + id + "'")){
        cout<<"Book with ID "<<id<<" not found. Enter any key: ";
        string x; cin>>x;
        return;
    }

    string name, author, isbn, publish_year;
    int edition, cop;
    cout<<"\t\t\t\t\t\tEnter New Book Name    : "; cin>>name;
    cout<<"\t\t\t\t\t\tEnter New Author Name  : "; cin>>author;
    cout<<"\t\t\t\t\t\tNew ISBN Code          : "; cin>>isbn;
    cout<<"\t\t\t\t\t\tNew Publish Year       : "; cin>>publish_year;
    cout<<"\t\t\t\t\t\tNew Edition(number)   : "; cin>>edition;
    cout<<"\t\t\t\t\t\tNew copies             : "; cin>>cop;

    stringstream ss;
    ss<<"UPDATE books SET "
      <<"name = '"<<name<<"', "
      <<"author = '"<<author<<"', "
      <<"isbn = '"<<isbn<<"', "
      <<"publish_year = '"<<publish_year<<"', "
      <<"edition = "<<edition<<", "
      <<"copies = "<<cop<<" "
      <<"WHERE book_id = '"<<id<<"'";
    
    exec_query(ss.str());
    cout<<"Book updated successfully. Enter any key: ";
    string x; cin>>x;
}

void Delete_Book(){
    system("cls");
    string id;
    cout<<"Enter -1 to return\n";
    cout<<"\t\t\t\t\t\tEnter Book ID to Delete: "; cin>>id;
    if(id == "-1")return;

    if(!check_exists("SELECT * FROM books WHERE book_id = '" + id + "'")){
        cout<<"Book with ID "<<id<<" not found. Enter any key: ";
        string x; cin>>x;
        return;
    }

    // Note: Due to "ON DELETE CASCADE" in our 'loans' table setup,
    // deleting a book will automatically delete all loan records for it.
    exec_query("DELETE FROM books WHERE book_id = '" + id + "'");

    cout<<"Book deleted successfully. Enter any key: ";
    string x; cin>>x;
}

void Display_All_User(){
    system("cls");
    cout<<"\t\t\t\t\t\t--- ALL USERS ---\n\n";

    if(mysql_query(conn, "SELECT * FROM users")){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* result = mysql_store_result(conn);
    
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        cout<<"\t\t\t\t\t\tID       : "<<row[0]<<endl;
        cout<<"\t\t\t\t\t\tNAME     : "<<row[1]<<endl;
        cout<<"\t\t\t\t\t\tPASSWORD : "<<row[2]<<endl;
        cout<<"\t\t\t\t\t\tPHONE    : "<<row[3]<<"\n\n";
    }

    mysql_free_result(result);
    cout<<"Enter 0 to return: ";
    string x; cin>>x;
}

void Search_User(){
    system("cls");
    string id;
    cout<<"\t\t\t\t\t\tEnter User ID : "; cin>>id;

    string query = "SELECT * FROM users WHERE user_id = '" + id + "'";
    if(mysql_query(conn, query.c_str())){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* result = mysql_store_result(conn);

    if(mysql_num_rows(result)== 0){
        cout<<"User NOT FOUND\n";
    }else{
        MYSQL_ROW row = mysql_fetch_row(result);
        cout<<"\t\t\t\t\t\tID       : "<<row[0]<<endl;
        cout<<"\t\t\t\t\t\tNAME     : "<<row[1]<<endl;
        cout<<"\t\t\t\t\t\tPASSWORD : "<<row[2]<<endl;
        cout<<"\t\t\t\t\t\tPHONE    : "<<row[3]<<"\n\n";
    }

    mysql_free_result(result);
    cout<<"Enter 0 to return: ";
    string x; cin>>x;
}

void Modify_User(){
    system("cls");
    string id, name, pass, phone;
    cout<<"Enter -1 to return\n";
    cout<<"\t\t\t\t\t\tEnter User ID to Modify: "; cin>>id;
    if(id == "-1")return;

    if(!check_exists("SELECT * FROM users WHERE user_id = '" + id + "'")){
        cout<<"User with ID "<<id<<" not found. Enter any key: ";
        string x; cin>>x;
        return;
    }

    cout<<"\t\t\t\t\t\tEnter New Name     : "; cin>>name;
    cout<<"\t\t\t\t\t\tEnter New Password : "; cin>>pass;
    cout<<"\t\t\t\t\t\tEnter New Phone    : +91 "; cin>>phone;

    stringstream ss;
    ss<<"UPDATE users SET "
       <<"name = '"<<name<<"', "
       <<"password = '"<<pass<<"', "
       <<"phone = '"<<phone<<"' "
       <<"WHERE user_id = '"<<id<<"'";
    
    exec_query(ss.str());
    cout<<"User updated successfully. Enter any key: ";
    string x; cin>>x;
}

void Delete_User(){
    system("cls");
    string id;
    cout<<"Enter -1 to return\n";
    cout<<"\t\t\t\t\t\tEnter User ID to Delete: "; cin>>id;
    if(id == "-1")return;

    if(!check_exists("SELECT * FROM users WHERE user_id = '" + id + "'")){
        cout<<"User with ID "<<id<<" not found. Enter any key: ";
        string x; cin>>x;
        return;
    }

    // Note: Due to "ON DELETE CASCADE" in our 'loans' table setup,
    // deleting a user will automatically delete all their loan records.
    exec_query("DELETE FROM users WHERE user_id = '" + id + "'");

    cout<<"User deleted successfully. Enter any key: ";
    string x; cin>>x;
}

void Display_All_Loan(){
    system("cls");
    cout<<"\t\t\t\t\t\t--- ALL ISSUED LOANS ---\n\n";
    
    string query = "SELECT l.loan_id, b.name, u.name, l.copies, l.issue_date "
                   "FROM loans l "
                   "JOIN users u ON l.user_id = u.user_id "
                   "JOIN books b ON l.book_id = b.book_id";

    if(mysql_query(conn, query.c_str())){
        throw runtime_error(mysql_error(conn));
    }
    MYSQL_RES* result = mysql_store_result(conn);
    
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        cout<<"\t\t\t\t\t\tLOAN ID    : "<<row[0]<<endl;
        cout<<"\t\t\t\t\t\tBOOK NAME  : "<<row[1]<<endl;
        cout<<"\t\t\t\t\t\tUSER NAME  : "<<row[2]<<endl;
        cout<<"\t\t\t\t\t\tCOPIES     : "<<row[3]<<endl;
        cout<<"\t\t\t\t\t\tISSUE DATE : "<<row[4]<<"\n\n";
    }

    mysql_free_result(result);
    cout<<"Enter 0 to return: ";
    string x; cin>>x;
}


// ############################################################
// ################# MAIN MENU / LOGIN FLOW ###################
// ############################################################

bool User::Add_User(bool from_admin){
    string res;
    system("cls");
    cout<<"Enter -1 to return in any below field\n";
    string name, password, phone;

    cout<<"\t\t\t\t\t\t--- USER SIGNUP ---\n";
    cout<<"\t\t\t\t\t\tEnter UserName : "; cin>>name; if(name == "-1")return 0;
    cout<<"\t\t\t\t\t\tEnter password : "; cin>>password; if(password == "-1")return 0;
    cout<<"\t\t\t\t\t\tPhone : +91 "; cin>>phone; if(phone == "-1")return 0;

    if(phone.size()!= 10){ 
        cout<<"\nInvalid Phone Number. Must be 10 digits. Enter any key: ";
        cin>>res;
        return 0; 
    }

    // Check if user already exists
    if(check_exists("SELECT * FROM users WHERE name = '" + name + "'")){
        cout<<"\nAccount Already Created - Enter Any key: ";
        cin>>res;
        return 0;
    }

    // Create new user
    string id = "A" + to_string(dist(gen));
    stringstream ss;
    ss<<"INSERT INTO users(user_id, name, password, phone)"
      <<"VALUES('"<<id<<"', '"<<name<<"', '"<<password<<"', '"<<phone<<"')";
    exec_query(ss.str());

    cout<<"Account created successfully!\n";

    if(!from_admin){
        // Automatically log in the new user
        User u(id, name, password, phone);
        u.User_Main_Menu();
    }else{
        cout<<"Press any key to return to admin menu: ";
        cin>>res;
    }

    return 1;
}

void User::Login(){
    system("cls");
    string name, password;
    cout<<"\t\t\t\t\t\t--- USER LOGIN ---\n";
    cout<<"\t\t\t\t\t\tEnter your UserName(no space): "; cin>>name;
    cout<<"\t\t\t\t\t\tEnter your password(no space): "; cin>>password;

    stringstream ss;
    ss<<"SELECT * FROM users WHERE name = '"<<name 
      <<"' AND password = '"<<password<<"'";

    if(mysql_query(conn, ss.str().c_str())){
        throw runtime_error(mysql_error(conn));
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if(mysql_num_rows(result)== 0){
        // No user found
        cout<<"\n\nUser Not Found or Incorrect Password. Enter Any Key: ";
        string x; cin>>x;
    }else{
        // User found, fetch data
        MYSQL_ROW row = mysql_fetch_row(result);
        // Create a User object and open their menu
        User logged_in_user(row[0], row[1], row[2], row[3]);
        logged_in_user.User_Main_Menu();
    }
    mysql_free_result(result);
}

void Administration(){
    system("cls");
    cout<<"\t########################################  ADMINISTRATION  ########################################\n";
    cout<<"\t\t\t\t\t\t1.  Show All Users\n";
    cout<<"\t\t\t\t\t\t2.  Search User\n";
    cout<<"\t\t\t\t\t\t3.  Add User\n";
    cout<<"\t\t\t\t\t\t4.  Delete User\n";
    cout<<"\t\t\t\t\t\t5.  Modify User\n";
    cout<<"\t\t\t\t\t\t6.  Show All Books\n";
    cout<<"\t\t\t\t\t\t7.  Add Books\n";
    cout<<"\t\t\t\t\t\t8.  Delete Book\n";
    cout<<"\t\t\t\t\t\t9.  Modify Book\n";
    cout<<"\t\t\t\t\t\t10. All Issued Books\n";
    cout<<"\t\t\t\t\t\t0.  Return to Main Menu\n";
    cout<<"\t\t\t\t\t\tEnter Option from [0-10] ";
    
    int opt; cin>>opt;
    string res;

    try{
        switch(opt){
        case 1: Display_All_User(); break;
        case 2: Search_User(); break;
        case 3: User::Add_User(true); break; // true = from_admin
        case 4: Delete_User(); break;
        case 5: Modify_User(); break;
        case 6: Display_All_Book(); break;
        case 7: Admin_Add_Book(); break;
        case 8: Delete_Book(); break;
        case 9: Modify_Book(); break;
        case 10: Display_All_Loan(); break;
        case 0: return; // Return to main menu
        }
    }catch(const exception& e){
        cout<<"An error occurred: "<<e.what()<<"\nEnter any key to continue: ";
        cin>>res;
    }
    
    if(opt != 0){
        Administration(); // Re-show admin menu
    }
}

int Main_Menu(){
    system("cls");
    int exit_code = 0;
    int option;
    cout<<"\t######################################  LIBRARY MANAGEMENT(MySQL) ######################################\n"<<endl;
    cout<<"\t\t\t\t\t\t 1. User Login "<<endl;
    cout<<"\t\t\t\t\t\t 2. User SignIn "<<endl;
    cout<<"\t\t\t\t\t\t 3. Administration"<<endl;
    cout<<"\t\t\t\t\t\t 4. EXIT"<<endl;
    cout<<"\t\t\t\t\t\t Enter option from [1-4]: "; cin>>option;
    string key;

    try{
        switch(option){
        case 1:
            User::Login();
            break;
        case 2:
            User::Add_User(false); // false = not from_admin
            break;
        case 3:
            system("cls");
            cout<<"Enter key 0 to return\n";
            cout<<"Enter Access Key(Key : AAYUSH): "; cin>>key;
            if(key == "0"){
                break;
            }
            if(key == "AAYUSH"){
                Administration();
            }
            break;
        case 4:
            exit_code = 1;
            break;
        default:
            cout<<"\n\n\nInValid Option\n"<<endl;
        }
    }catch(const exception& e){
        cout<<"\n\nCRITICAL ERROR: "<<e.what()<<"\nPress any key to exit.";
        string x; cin>>x;
        exit_code = 1; // Exit on critical error
    }

    return exit_code;
}

// ############################################################
// ###################### MAIN FUNCTION #######################
// ############################################################

int main(){
    // 1. Initialize the connection object
    conn = mysql_init(NULL);
    if(conn == NULL){
        cerr<<"Error: mysql_init()failed"<<endl;
        return 1;
    }

    // 2. Extract the password
    string db_password = "";
    ifstream env_file(".env"); 
    if(!env_file.is_open()){
        cerr<<"Error: Could not open .env file."<<endl;
        cerr<<"Please create a .env file and add your database password to it."<<endl;
        return 1;
    }
    getline(env_file, db_password);
    env_file.close();

    if(db_password.empty()){
        cerr<<"Error: .env file is empty. Please add your password."<<endl;
        return 1;
    }

    // 3. Connect to the database
    if(mysql_real_connect(conn, HOST, USER, db_password.c_str(), DB, 0, NULL, 0)== NULL){
        cerr<<"Error: mysql_real_connect()failed:"<<endl;
        cerr<<"  "<<mysql_error(conn)<<endl;
        mysql_close(conn);
        return 1;
    }

    cout<<"Successfully connected to MySQL database: "<<DB<<endl;

    // 4. Run the main application loop
    int exit_code = 0;
    system("cls");
    while(!exit_code){
        exit_code = Main_Menu();
    }

    // 5. Clean up
    cout<<"Thanks for using the system. Connection closed."<<endl;
    mysql_close(conn);

    return 0;
}