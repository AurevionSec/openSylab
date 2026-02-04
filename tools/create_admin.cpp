#include <iostream>
#include <sqlite3.h>
#include "../include/core/User.h"
#include <ctime>

int main() {
    // Create admin user
    opensylab::core::User admin("admin", "", opensylab::core::User::Role::ADMIN);
    admin.setPassword("admin");

    // Open database
    sqlite3* db;
    int rc = sqlite3_open("opensylab.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // Insert admin user
    std::string sql = "INSERT INTO users (username, password_hash, role, active, created_date) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    std::time_t now = std::time(nullptr);
    sqlite3_bind_text(stmt, 1, admin.getUsername().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, admin.getPasswordHash().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, "ADMIN", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, 1);
    sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(now));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert admin: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    std::cout << "Admin user created successfully!" << std::endl;
    std::cout << "Username: admin" << std::endl;
    std::cout << "Password: admin" << std::endl;

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
