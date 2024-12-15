#include <iostream>
#include <string>
#include <mysql.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

const char *HOST = "localhost";
const char *USER = "root";
const char *PASSWORD = "";
const char *DATABASE = "oop";

int stoi(const string &str)
{
    int result;
    stringstream ss(str);
    ss >> result;
    return result;
}

void toLowerCase(string &str)
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}

class Books
{
public:
    // Add a book to the library
    void addBook(MYSQL *conn, string &name, string &author, int quantity)
    {
        toLowerCase(name);
        toLowerCase(author);

        stringstream ss;
        ss << "INSERT INTO BOOKS (NAME, AUTHOR, QUANTITY) VALUES ('" << name << "', '" << author << "', " << quantity << ")";
        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Add Book Error: " << mysql_error(conn) << endl;
        }
        else
        {
            cout << "Book added successfully." << endl;
        }
    }

    // Delete a book from the library
    void deleteBook(MYSQL *conn, string &name)
    {
        toLowerCase(name);

        stringstream ss;
        ss << "DELETE FROM BOOKS WHERE NAME = '" << name << "'LIMIT 1";

        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Error deleting book: " << mysql_error(conn) << endl;
        }
        else
        {
            if (mysql_affected_rows(conn) > 0)
            {
                cout << "Book deleted successfully." << endl;
            }
            else
            {
                cout << "No such book found in the library." << endl;
            }
        }
    }

    // Search for books
    void searchBook(MYSQL *conn, string &name)
    {
        toLowerCase(name);

        stringstream ss;
        ss << "SELECT * FROM BOOKS WHERE NAME LIKE '%" << name << "%'";
        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Search Book Error: " << mysql_error(conn) << endl;
        }
        else
        {
            MYSQL_RES *res = mysql_store_result(conn);
            if (res)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)))
                {
                    cout << "Book ID: " << row[0] << ", Name: " << row[1] << ", Author: " << row[2] << ", Quantity: " << row[3] << endl;
                }
                mysql_free_result(res);
            }
        }
    }

    // Display all books in the library
    void displayAllBooks(MYSQL *conn)
    {
        if (mysql_query(conn, "SELECT * FROM BOOKS"))
        {
            cerr << "Error retrieving books: " << mysql_error(conn) << endl;
        }
        else
        {
            MYSQL_RES *res = mysql_store_result(conn);
            if (res)
            {
                MYSQL_ROW row;
                cout << left
                     << setw(10) << "Book ID"
                     << setw(30) << "Name"
                     << setw(20) << "Author"
                     << setw(10) << "Quantity"
                     << endl;
                cout << std::string(70, '-') << std::endl;

                while ((row = mysql_fetch_row(res)))
                {
                    cout << left
                         << setw(10) << row[0] << setw(30) << row[1] << setw(20) << row[2] << setw(10) << row[3] << endl;
                }
                mysql_free_result(res);
            }
        }
    }

    // Issue a book to a student
    void issueBook(MYSQL *conn, string &roll_num, string &book_name)
    {
        toLowerCase(roll_num);
        toLowerCase(book_name);

        // Check if roll number exists in STUDENTS table
        stringstream ssCheckRoll;
        ssCheckRoll << "SELECT ROLL_NUM FROM STUDENTS WHERE ROLL_NUM = '" << roll_num << "'";

        if (mysql_query(conn, ssCheckRoll.str().c_str()))
        {
            cerr << "Wrong Roll Num: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK");
            return;
        }

        MYSQL_RES *resRoll = mysql_store_result(conn);
        if (resRoll)
        {
            if (mysql_num_rows(resRoll) == 0)
            {
                cerr << "Roll number not found!" << endl;
                mysql_free_result(resRoll); // Free result set
                mysql_query(conn, "ROLLBACK");
                return;
            }
            mysql_free_result(resRoll); // Free result set
        }
        else
        {
            cerr << "Error fetching roll number: " << mysql_error(conn) << endl;
            return;
        }

        // Check if the book exists and fetch its ID and quantity
        stringstream ssCheck;
        ssCheck << "SELECT ID, QUANTITY FROM BOOKS WHERE NAME LIKE '%" << book_name << "%'";

        if (mysql_query(conn, ssCheck.str().c_str()))
        {
            cerr << "Error checking book: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK"); // Rollback on error
            return;
        }

        MYSQL_RES *resBook = mysql_store_result(conn);
        if (resBook)
        {
            MYSQL_ROW row = mysql_fetch_row(resBook);
            if (row)
            {
                int book_id = stoi(row[0]);
                int quantity = stoi(row[1]);

                if (quantity > 0)
                {
                    // Check if the student has already issued the book
                    stringstream ssCheckIssued;
                    ssCheckIssued << "SELECT * FROM BOOKS_ISSUED WHERE ROLL_NUM = '" << roll_num << "' AND ID = " << book_id;

                    if (mysql_query(conn, ssCheckIssued.str().c_str()))
                    {
                        cerr << "Error checking issued book: " << mysql_error(conn) << endl;
                        mysql_query(conn, "ROLLBACK");
                        return;
                    }

                    MYSQL_RES *resIssued = mysql_store_result(conn);
                    if (resIssued)
                    {
                        if (mysql_num_rows(resIssued) > 0)
                        {
                            cout << "Error: You have already issued this book." << endl;
                            mysql_free_result(resIssued); // Free result set
                            mysql_free_result(resBook);   // Free result set
                            mysql_query(conn, "ROLLBACK");
                            return;
                        }
                        mysql_free_result(resIssued); // Free result set
                    }

                    // Proceed with issuing the book
                    stringstream ssIssue;
                    ssIssue << "INSERT INTO BOOKS_ISSUED (ID, ROLL_NUM) VALUES (" << book_id << ", '" << roll_num << "')";

                    if (mysql_query(conn, ssIssue.str().c_str()))
                    {
                        cerr << "Error issuing book: " << mysql_error(conn) << endl;
                        mysql_query(conn, "ROLLBACK"); // Rollback on error
                        return;
                    }
                    else
                    {
                        // Decrease the quantity of the book
                        stringstream ssUpdate;
                        ssUpdate << "UPDATE BOOKS SET QUANTITY = QUANTITY - 1 WHERE ID = " << book_id;

                        if (mysql_query(conn, ssUpdate.str().c_str()))
                        {
                            cerr << "Error updating book quantity: " << mysql_error(conn) << endl;
                            mysql_query(conn, "ROLLBACK"); // Rollback on error
                            return;
                        }
                        else
                        {
                            mysql_query(conn, "COMMIT"); // Commit transaction
                            cout << "Book issued successfully." << endl;
                        }
                    }
                }
                else
                {
                    cout << "Book not available in sufficient quantity." << endl;
                    mysql_free_result(resBook); // Free result set
                    mysql_query(conn, "ROLLBACK");
                }
            }
            else
            {
                cerr << "Book not found!" << endl;
                mysql_free_result(resBook); // Free result set
                mysql_query(conn, "ROLLBACK");
            }
            mysql_free_result(resBook); // Free result set
        }
        else
        {
            cerr << "Error fetching book data: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK");
        }
    }

    // Return a book
    void returnBook(MYSQL *conn, string &roll_num, string &book_name)
    {

        toLowerCase(roll_num);
        toLowerCase(book_name);

        mysql_query(conn, "START TRANSACTION"); // Start transaction

        stringstream ssCheckRoll;
        ssCheckRoll << "SELECT ROLL_NUM FROM STUDENTS WHERE ROLL_NUM = '" << roll_num << "'";

        if (mysql_query(conn, ssCheckRoll.str().c_str()))
        {
            cerr << "Wrong Roll Num: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK");
        }

        MYSQL_RES *resRoll = mysql_store_result(conn);
        if (resRoll)
        {
            if (mysql_num_rows(resRoll) == 0)
            {
                cerr << "Roll number not found!" << endl;
                mysql_free_result(resRoll); // Free result set
                mysql_query(conn, "ROLLBACK");
            }
            mysql_free_result(resRoll); // Free result set
        }
        else
        {
            cerr << "Error fetching roll number: " << mysql_error(conn) << endl;
        }

        stringstream ssCheck;
        ssCheck << "SELECT BOOKS_ISSUED.S_NO, BOOKS.ID, BOOKS_ISSUED.ISSUE_DATE FROM BOOKS_ISSUED "
                << "JOIN BOOKS ON BOOKS_ISSUED.ID = BOOKS.ID "
                << "WHERE BOOKS.NAME LIKE '%" << book_name << "%' AND BOOKS_ISSUED.ROLL_NUM = '" << roll_num << "'";

        if (mysql_query(conn, ssCheck.str().c_str()))
        {
            cerr << "Error checking issued book: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK"); // Rollback on error
        }

        MYSQL_RES *res = mysql_store_result(conn);
        if (res)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row)
            {
                int issue_id = stoi(row[0]);
                int book_id = stoi(row[1]);
                string issue_date = row[2]; // Assuming ISSUE_DATE is in format 'YYYY-MM-DD'

                // Calculate the fine for late return
                stringstream ssFineCheck;
                ssFineCheck << "SELECT DATEDIFF(CURDATE(), '" << issue_date << "') AS days_diff";

                if (mysql_query(conn, ssFineCheck.str().c_str()))
                {
                    cerr << "Error calculating fine: " << mysql_error(conn) << endl;
                    mysql_query(conn, "ROLLBACK");
                }

                MYSQL_RES *fineRes = mysql_store_result(conn);
                if (fineRes)
                {
                    MYSQL_ROW fineRow = mysql_fetch_row(fineRes);
                    if (fineRow)
                    {
                        int days_diff = stoi(fineRow[0]);
                        if (days_diff > 90)
                        {
                            int late_days = days_diff - 90;
                            int fine = late_days * 5; // 5 rupee per late day
                            cout << "Late submission! Fine for " << late_days << " late days: Rs. " << fine << endl;
                        }
                        else
                        {
                            cout << "No fine. Book returned within the allowed 90 days." << endl;
                        }
                    }
                    mysql_free_result(fineRes);
                }

                // Proceed with returning the book
                stringstream ssReturn;
                ssReturn << "DELETE FROM BOOKS_ISSUED WHERE S_NO = " << issue_id;

                if (mysql_query(conn, ssReturn.str().c_str()))
                {
                    cerr << "Error returning book: " << mysql_error(conn) << endl;
                    mysql_query(conn, "ROLLBACK"); // Rollback on error
                }
                else
                {
                    // Increase the quantity of the book
                    stringstream ssUpdate;
                    ssUpdate << "UPDATE BOOKS SET QUANTITY = QUANTITY + 1 WHERE ID = " << book_id;

                    if (mysql_query(conn, ssUpdate.str().c_str()))
                    {
                        cerr << "Error updating book quantity: " << mysql_error(conn) << endl;
                        mysql_query(conn, "ROLLBACK"); // Rollback on error
                    }
                    else
                    {
                        mysql_query(conn, "COMMIT"); // Commit transaction
                        cout << "Book returned successfully." << endl;
                    }
                }
            }
            else
            {
                cout << "No record of this book being issued to the student." << endl;
                mysql_query(conn, "ROLLBACK");
            }
            mysql_free_result(res);
        }
    }
};

class Admin
{
public:
    bool login(MYSQL *conn, const string &username, const string &password)
    {
        stringstream ss;
        ss << "SELECT * FROM ADMIN WHERE username = '" << username << "' AND password = '" << password << "'";
        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Login Error: " << mysql_error(conn) << endl;
            return false;
        }

        MYSQL_RES *res = mysql_store_result(conn);
        if (res)
        {
            if (mysql_num_rows(res) > 0)
            {
                cout << "Admin logged in successfully!" << endl;
                mysql_free_result(res);
                return true;
            }
            else
            {
                cout << "Invalid username or password." << endl;
            }
            mysql_free_result(res);
        }
        return false;
    }

    void adminOptions(MYSQL *conn)
    {
        Books books;
        int choice;
        bool exitMenu = false;

        while (!exitMenu)
        {
            cout << "\nAdmin Menu:\n";
            cout << "1. Add Book\n";
            cout << "2. Remove Book\n";
            cout << "3. Search Book\n";
            cout << "4. Display All Books\n";
            cout << "5. Issue Book\n";
            cout << "6. Return Book\n";
            cout << "7. All issued Books\n";
            cout << "8. Register New Student\n";
            cout << "9. Delete Student\n";
            cout << "10. Display All Students\n";
            cout << "11. Create New Admin\n";
            cout << "12. Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;

            switch (choice)
            {
            case 1:
            {
                string name, author;
                int quantity;
                cout << "Enter book name: ";
                cin >> name;
                cout << "Enter author name: ";
                cin >> author;
                cout << "Enter quantity: ";
                cin >> quantity;
                books.addBook(conn, name, author, quantity);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 2:
            {
                string name;
                cout << "Enter book name to delete: ";
                cin >> name;
                books.deleteBook(conn, name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 3:
            {
                string name;
                cout << "Enter book name to search: ";
                cin >> name;
                books.searchBook(conn, name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 4:
                books.displayAllBooks(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            case 5:
            {
                string roll_num, book_name;
                cout << "Enter student roll number: ";
                cin >> roll_num;
                cout << "Enter book name to issue: ";
                cin >> book_name;
                books.issueBook(conn, roll_num, book_name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 6:
            {
                string roll_num, book_name;
                cout << "Enter student roll number: ";
                cin >> roll_num;
                cout << "Enter book name to return: ";
                cin >> book_name;
                books.returnBook(conn, roll_num, book_name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 7:
            {
                viewAllBorrowedBooks(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 8:
            {
                registerStudent(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 9:
            {
                deleteStudent(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 10:
            {
                displayAllStudents(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 11:
            {
                createAdmin(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 12:
                exitMenu = true;
                break;
            default:
                cout << "Invalid choice! Please try again." << endl;
            }
        }
    }

    void createAdmin(MYSQL *conn)
    {
        string username, password;
        cout << "Enter new admin username: ";
        cin >> username;
        cout << "Enter new admin password: ";
        cin >> password;

        stringstream ss;
        ss << "INSERT INTO ADMIN (username, password) VALUES ('" << username << "', '" << password << "')";

        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Create Admin Error: " << mysql_error(conn) << endl;
        }
        else
        {
            cout << "New admin created successfully!" << endl;
        }
    }

    void viewAllBorrowedBooks(MYSQL *conn)
    {
        stringstream ss;
        ss << "SELECT BOOKS.NAME, BOOKS_ISSUED.ISSUE_DATE ,BOOKS_ISSUED.ROLL_NUM FROM BOOKS_ISSUED JOIN BOOKS ON BOOKS_ISSUED.ID = BOOKS.ID ";
        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "View Borrowed Books Error: " << mysql_error(conn) << endl;
        }
        else
        {
            MYSQL_RES *res = mysql_store_result(conn);
            if (res)
            {
                MYSQL_ROW row;
                cout << left
                     << setw(40) << "Book Name"
                     << setw(20) << "Roll Num"
                     << setw(20) << "Issue Date"
                     << endl;
                cout << string(80, '-') << endl;

                while ((row = mysql_fetch_row(res)))
                {
                    cout << left
                         << setw(40) << row[0] << setw(20) << row[1] << setw(20) << row[2] << endl;
                }
                mysql_free_result(res);
            }
        }
    }

    // Register new student
    void registerStudent(MYSQL *conn)
    {
        string roll_num, name;
        cout << "Enter Student Roll Number: ";
        cin >> roll_num;
        cout << "Enter Student Name: ";
        cin.ignore();
        getline(cin, name);

        toLowerCase(roll_num);
        toLowerCase(name);

        stringstream ss;
        ss << "INSERT INTO STUDENTS (ROLL_NUM, NAME) VALUES ('" << roll_num << "', '" << name << "')";
        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Error registering student: " << mysql_error(conn) << endl;
        }
        else
        {
            cout << "Student registered successfully." << endl;
        }
    }

    // Delete student
    void deleteStudent(MYSQL *conn)
    {

        string roll_num;
        cout << "Enter Student Roll Number to Delete: ";
        cin >> roll_num;

        toLowerCase(roll_num);

        stringstream ss;
        ss << "DELETE FROM STUDENTS WHERE ROLL_NUM = '" << roll_num << "' LIMIT 1";

        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "Error deleting student: " << mysql_error(conn) << endl;
        }
        else
        {
            if (mysql_affected_rows(conn) > 0)
            {
                cout << "Student deleted successfully." << endl;
            }
            else
            {
                cout << "No such student found in the database." << endl;
            }
        }
    }

    // Display all students
    void displayAllStudents(MYSQL *conn)
    {
        if (mysql_query(conn, "SELECT * FROM STUDENTS"))
        {
            cerr << "Error retrieving students: " << mysql_error(conn) << endl;
        }
        else
        {
            MYSQL_RES *res = mysql_store_result(conn);
            if (res)
            {
                MYSQL_ROW row;
                cout << left
                     << setw(20) << "Roll Num"
                     << setw(20) << "Name"
                     << endl;
                cout << string(70, '-') << endl;

                while ((row = mysql_fetch_row(res)))
                {
                    cout << left
                         << setw(20) << row[0] << setw(20) << row[1] << endl;
                }
                mysql_free_result(res);
            }
        }
    }
};

class Student
{
public:
    void studentOptions(MYSQL *conn)
    {
        Books books;
        int choice;
        bool exitMenu = false;

        while (!exitMenu)
        {
            cout << "\nStudent Menu:\n";
            cout << "1. Search Book\n";
            cout << "2. Display All Books\n";
            cout << "3. Issue Book\n";
            cout << "4. Return Book\n";
            cout << "5. All Issued Books\n";
            cout << "6. Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;

            switch (choice)
            {
            case 1:
            {
                string name;
                cout << "Enter book name to search: ";
                cin >> name;
                books.searchBook(conn, name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 2:
                books.displayAllBooks(conn);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            case 3:
            {
                string roll_num, book_name;
                cout << "Enter your roll number: ";
                cin >> roll_num;
                cout << "Enter book name to issue: ";
                cin >> book_name;
                books.issueBook(conn, roll_num, book_name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 4:
            {
                string roll_num, book_name;
                cout << "Enter your roll number: ";
                cin >> roll_num;
                cout << "Enter book name to return: ";
                cin >> book_name;
                books.returnBook(conn, roll_num, book_name);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 5:
            {
                string roll_num, book_name;
                cout << "Enter your roll number: ";
                cin >> roll_num;
                viewBorrowedBooks(conn, roll_num);
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
            case 6:
                exitMenu = true;
                break;
            default:
                cout << "Invalid choice! Please try again." << endl;
            }
        }
    }
    void viewBorrowedBooks(MYSQL *conn, string &roll_num)
    {
        toLowerCase(roll_num);

        stringstream ss;
        ss << "SELECT BOOKS.NAME, BOOKS_ISSUED.ISSUE_DATE FROM BOOKS_ISSUED JOIN BOOKS ON BOOKS_ISSUED.ID = BOOKS.ID WHERE BOOKS_ISSUED.ROLL_NUM = '" << roll_num << "'";
        if (mysql_query(conn, ss.str().c_str()))
        {
            cerr << "View Borrowed Books Error: " << mysql_error(conn) << endl;
        }
        else
        {
            MYSQL_RES *res = mysql_store_result(conn);
            if (res)
            {
                MYSQL_ROW row;
                cout << left
                     << setw(20) << "Book Name"
                     << setw(20) << "Issue Date"
                     << endl;
                cout << string(70, '-') << endl;

                while ((row = mysql_fetch_row(res)))
                {
                    cout << left
                         << setw(20) << row[0] << setw(20) << row[1] << endl;
                }
                mysql_free_result(res);
            }
        }
    }
};

// Main function to establish connection and start the program
int main()
{
    MYSQL *conn;
    conn = mysql_init(0);
    if (conn)
    {
        if (mysql_real_connect(conn, HOST, USER, PASSWORD, DATABASE, 3306, NULL, 0))
        {
            int userType;
            cout << "Welcome to Library Management System!\n";
            cout << "Are you logging in as:\n";
            cout << "1. Admin\n";
            cout << "2. Student\n";
            cout << "Enter your choice: ";
            cin >> userType;

            if (userType == 1)
            {
                Admin admin;
                string username, password;
                cout << "Enter Admin username: ";
                cin >> username;
                cout << "Enter Admin password: ";
                cin >> password;

                if (admin.login(conn, username, password))
                {
                    admin.adminOptions(conn);
                }
            }
            else if (userType == 2)
            {
                Student student;
                student.studentOptions(conn);
            }
            else
            {
                cout << "Invalid user type!" << endl;
            }
        }
        else
        {
            cout << "Database connection failed: " << mysql_error(conn) << endl;
        }
    }
    else
    {
        cout << "MySQL initialization failed!" << endl;
    }

    return 0;
}
