#!/usr/bin/env python3
"""
OpenSylab Test Data Generator
Generates realistic test data for LIMS demonstration
"""

import sqlite3
import random
import hashlib
import os
import base64
from datetime import datetime, timedelta

# Configuration
NUM_PATIENTS = 116
NUM_SAMPLES = 524
NUM_ORDERS = 351
NUM_USERS = 10
NUM_RESULTS = 450  # Results per order varies

# Database path
DB_PATH = "opensylab.db"

# Sample data pools
FIRST_NAMES = ["Emma", "Liam", "Olivia", "Noah", "Ava", "Ethan", "Sophia", "Mason", "Isabella", "William",
               "Mia", "James", "Charlotte", "Benjamin", "Amelia", "Lucas", "Harper", "Henry", "Evelyn", "Alexander",
               "Maria", "Hans", "Anna", "Klaus", "Sophie", "Michael", "Laura", "Thomas", "Julia", "Stefan",
               "Petra", "Wolfgang", "Sabine", "Christian", "Monika", "Andreas", "Gabriele", "Frank", "Martina", "Jürgen",
               "Lisa", "Max", "Sarah", "Felix", "Nina", "Paul", "Lea", "Leon", "Hannah", "Tim"]

LAST_NAMES = ["Schmidt", "Müller", "Schneider", "Fischer", "Weber", "Meyer", "Wagner", "Becker", "Schulz", "Hoffmann",
              "Schäfer", "Koch", "Bauer", "Richter", "Klein", "Wolf", "Schröder", "Neumann", "Schwarz", "Zimmermann",
              "Braun", "Krüger", "Hofmann", "Hartmann", "Lange", "Schmitt", "Werner", "Schmitz", "Krause", "Meier",
              "Lehmann", "Schmid", "Schulze", "Maier", "Köhler", "Herrmann", "Walter", "König", "Huber", "Kaiser"]

TEST_TYPES = [
    "Blutbild", "CRP", "Leber-Enzyme", "Nieren-Werte", "Elektrolyte",
    "Blutzucker", "HbA1c", "Schilddrüse", "Lipid-Profil", "Urinstatus",
    "D-Dimer", "Troponin", "BNP", "PSA", "Vitamin D", "Ferritin",
    "TSH", "Cortisol", "HIV-Test", "Hepatitis-Panel"
]

TEST_PARAMETERS = {
    "Blutbild": [
        ("Hämoglobin", "g/dL", 13.5, 17.5),
        ("Erythrozyten", "Mill/µL", 4.5, 5.9),
        ("Leukozyten", "Tsd/µL", 4.0, 10.0),
        ("Thrombozyten", "Tsd/µL", 150, 400)
    ],
    "CRP": [("CRP", "mg/L", 0, 5)],
    "Leber-Enzyme": [
        ("ALT", "U/L", 10, 50),
        ("AST", "U/L", 10, 50),
        ("GGT", "U/L", 10, 60)
    ],
    "Nieren-Werte": [
        ("Kreatinin", "mg/dL", 0.7, 1.3),
        ("Harnstoff", "mg/dL", 10, 50),
        ("GFR", "mL/min", 90, 120)
    ],
    "Elektrolyte": [
        ("Natrium", "mmol/L", 135, 145),
        ("Kalium", "mmol/L", 3.5, 5.0),
        ("Calcium", "mmol/L", 2.2, 2.6)
    ],
    "Blutzucker": [("Glucose", "mg/dL", 70, 100)],
    "HbA1c": [("HbA1c", "%", 4.0, 6.0)],
    "Schilddrüse": [
        ("TSH", "mU/L", 0.4, 4.0),
        ("fT4", "ng/dL", 0.8, 1.8),
        ("fT3", "pg/mL", 2.3, 4.2)
    ],
    "Lipid-Profil": [
        ("Cholesterin", "mg/dL", 120, 200),
        ("LDL", "mg/dL", 0, 130),
        ("HDL", "mg/dL", 40, 60),
        ("Triglyceride", "mg/dL", 0, 150)
    ]
}

SAMPLE_STATUSES = ["REGISTERED", "IN_ANALYSIS", "ANALYZED", "VALIDATED", "ARCHIVED"]
ORDER_STATUSES = ["REQUESTED", "IN_PROGRESS", "COMPLETED", "VALIDATED", "CANCELLED"]
ORDER_PRIORITIES = ["NORMAL", "URGENT", "EMERGENCY"]
RESULT_STATUSES = ["PENDING", "ENTERED", "VALIDATED", "REJECTED", "REPEATED"]
RESULT_FLAGS = ["NORMAL", "LOW", "HIGH", "CRITICAL"]

USERNAMES = ["admin", "labor1", "labor2", "labor3", "arzt1", "arzt2", "validierung", "eingabe", "empfang", "leitung"]
USER_ROLES = ["ADMIN", "TECHNICIAN", "TECHNICIAN", "TECHNICIAN", "DOCTOR", "DOCTOR", "ADMIN", "TECHNICIAN", "TECHNICIAN", "ADMIN"]

def hash_password(password: str) -> str:
    """Generate a password hash matching the backend format
    (src/core/User.cpp): pbkdf2_sha256$iterations$saltB64$hashB64.

    Uses PBKDF2-HMAC-SHA256 with the same parameters as the C++ backend so
    generated test users can authenticate, and to avoid using a fast,
    non-iterated hash on credentials.
    """
    iterations = 210000  # OWASP recommendation for PBKDF2-SHA256 (matches backend)
    salt = os.urandom(16)  # 128-bit salt
    derived = hashlib.pbkdf2_hmac("sha256", password.encode(), salt, iterations, dklen=32)
    salt_b64 = base64.b64encode(salt).decode("ascii")
    hash_b64 = base64.b64encode(derived).decode("ascii")
    return f"pbkdf2_sha256${iterations}${salt_b64}${hash_b64}"

def random_date(start_days_ago: int, end_days_ago: int = 0) -> int:
    """Generate random Unix timestamp"""
    start = datetime.now() - timedelta(days=start_days_ago)
    end = datetime.now() - timedelta(days=end_days_ago)
    random_date = start + (end - start) * random.random()
    return int(random_date.timestamp())

def generate_patient_id(index: int) -> str:
    """Generate patient ID"""
    return f"P{index:04d}"

def generate_sample_id(index: int) -> str:
    """Generate sample ID"""
    return f"S{index:03d}"

def generate_order_id(index: int) -> str:
    """Generate order ID"""
    return f"O{index:04d}"

def generate_result_id(index: int) -> str:
    """Generate result ID"""
    return f"R{index:05d}"

def generate_test_value(ref_low: float, ref_high: float, make_abnormal: float = 0.15) -> tuple:
    """Generate test value and flag"""
    if random.random() < make_abnormal:
        # Generate abnormal value
        if random.random() < 0.5:
            # Low
            value = ref_low * random.uniform(0.5, 0.95)
            flag = "LOW" if value > ref_low * 0.7 else "CRITICAL"
        else:
            # High
            value = ref_high * random.uniform(1.05, 1.5)
            flag = "HIGH" if value < ref_high * 1.3 else "CRITICAL"
    else:
        # Generate normal value
        value = random.uniform(ref_low, ref_high)
        flag = "NORMAL"

    return round(value, 2), flag

def main():
    print("🔬 OpenSylab Test Data Generator")
    print("=" * 50)

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # Check if we should clear existing data
    cursor.execute("SELECT COUNT(*) FROM samples")
    existing_samples = cursor.fetchone()[0]

    if existing_samples > 1:
        print(f"\n⚠️  Warnung: {existing_samples} Samples bereits vorhanden!")
        print("Daten werden HINZUGEFÜGT (nicht überschrieben)")
        input("Enter drücken zum Fortfahren oder Ctrl+C zum Abbrechen...")

    print(f"\n📊 Generiere Testdaten:")
    print(f"  - {NUM_USERS} Benutzer")
    print(f"  - {NUM_PATIENTS} Patienten")
    print(f"  - {NUM_SAMPLES} Proben")
    print(f"  - {NUM_ORDERS} Aufträge")
    print(f"  - ~{NUM_RESULTS} Ergebnisse")
    print()

    # 1. Generate Users
    print("👥 Generiere Benutzer...")
    for i in range(NUM_USERS):
        username = USERNAMES[i]
        password_hash = hash_password("password")  # Simple password for demo
        role = USER_ROLES[i]

        cursor.execute("""
            INSERT OR IGNORE INTO users (username, password_hash, role, created_date)
            VALUES (?, ?, ?, ?)
        """, (username, password_hash, role, int(datetime.now().timestamp())))

    print(f"   ✓ {NUM_USERS} Benutzer erstellt")

    # 2. Generate Patients and Samples
    print("🧪 Generiere Patienten und Proben...")
    patients = []
    samples = []

    for i in range(NUM_PATIENTS):
        patient_id = generate_patient_id(i + 1)
        patient_name = f"{random.choice(FIRST_NAMES)} {random.choice(LAST_NAMES)}"
        patients.append((patient_id, patient_name))

    for i in range(NUM_SAMPLES):
        sample_id = generate_sample_id(i + 1)
        patient_id, patient_name = random.choice(patients)
        description = random.choice(["Serum", "EDTA-Blut", "Plasma", "Urin", "Vollblut"])
        status = random.choices(
            SAMPLE_STATUSES,
            weights=[5, 15, 30, 40, 10],  # More validated/analyzed samples
            k=1
        )[0]
        registration_date = random_date(90, 0)  # Last 90 days

        cursor.execute("""
            INSERT INTO samples (sample_id, patient_id, patient_name, description, status, registration_date)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (sample_id, patient_id, patient_name, description, status, registration_date))

        samples.append((sample_id, registration_date))

    print(f"   ✓ {NUM_PATIENTS} Patienten und {NUM_SAMPLES} Proben erstellt")

    # 3. Generate Orders
    print("📋 Generiere Aufträge...")
    orders = []

    for i in range(NUM_ORDERS):
        order_id = generate_order_id(i + 1)
        sample_id, sample_date = random.choice(samples)
        test_type = random.choice(TEST_TYPES)
        status = random.choices(
            ORDER_STATUSES,
            weights=[10, 20, 50, 15, 5],  # More completed orders
            k=1
        )[0]
        priority = random.choices(
            ORDER_PRIORITIES,
            weights=[70, 25, 5],
            k=1
        )[0]
        requested_date = sample_date + random.randint(0, 3600)  # Up to 1 hour after sample
        completed_date = None
        if status in ["COMPLETED", "VALIDATED"]:
            completed_date = requested_date + random.randint(3600, 86400 * 3)  # 1-3 days later

        requested_by = random.choice(["Dr. Schmidt", "Dr. Müller", "Dr. Wagner", "Dr. Becker", "Dr. Hoffmann"])
        notes = random.choice([None, None, None, "Nüchternprobe", "Kontrolle", "Eilig", "Nachforderung"])

        cursor.execute("""
            INSERT INTO orders (order_id, sample_id, test_type, status, priority, requested_date, completed_date, requested_by, notes)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (order_id, sample_id, test_type, status, priority, requested_date, completed_date, requested_by, notes))

        order_db_id = cursor.lastrowid
        orders.append((order_db_id, order_id, test_type, status, requested_date))

    print(f"   ✓ {NUM_ORDERS} Aufträge erstellt")

    # 4. Generate Test Results
    print("📊 Generiere Testergebnisse...")
    result_count = 0

    for order_db_id, order_id, test_type, order_status, order_date in orders:
        # Only generate results for completed/validated orders
        if order_status not in ["COMPLETED", "VALIDATED", "IN_PROGRESS"]:
            continue

        # Get test parameters for this test type
        if test_type not in TEST_PARAMETERS:
            # Use generic parameters for unknown test types
            parameters = [("Parameter", "unit", 0, 100)]
        else:
            parameters = TEST_PARAMETERS[test_type]

        for param_name, unit, ref_low, ref_high in parameters:
            result_id = generate_result_id(result_count + 1)
            value, flag = generate_test_value(ref_low, ref_high)
            reference_range = f"{ref_low} - {ref_high}"

            result_status = "VALIDATED" if order_status == "VALIDATED" else random.choice(["ENTERED", "VALIDATED"])
            measured_date = order_date + random.randint(3600, 86400)  # 1-24 hours after order
            measured_by = random.choice(USERNAMES[1:])  # Not admin
            comment = None if flag == "NORMAL" else random.choice([None, "Control recommended", "Finding abnormal", "Please repeat"])

            cursor.execute("""
                INSERT INTO test_results (result_id, order_id, test_parameter, value, unit, reference_range,
                                         reference_low, reference_high, status, flag, measured_date, measured_by, comment)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (result_id, order_db_id, param_name, str(value), unit, reference_range,
                  ref_low, ref_high, result_status, flag, measured_date, measured_by, comment))

            result_count += 1

    print(f"   ✓ {result_count} Testergebnisse erstellt")

    # 5. Generate Audit Log entries
    print("📝 Generiere Audit-Log-Einträge...")
    audit_entries = 0
    actions = ["CREATE", "UPDATE", "DELETE", "LOGIN", "LOGOUT", "VALIDATE"]

    for _ in range(200):  # 200 audit entries
        action = random.choice(actions)
        entity = random.choice(["Sample", "Order", "Result", "User"])
        entity_id = random.randint(1, 100)
        user = random.choice(USERNAMES)
        timestamp = random_date(90, 0)
        details = f"{action} {entity} #{entity_id}"

        cursor.execute("""
            INSERT INTO audit_log (action, entity, entity_id, user, timestamp, details)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (action, entity, entity_id, user, timestamp, details))

        audit_entries += 1

    print(f"   ✓ {audit_entries} Audit-Einträge erstellt")

    # Commit and close
    conn.commit()
    conn.close()

    print("\n" + "=" * 50)
    print("✅ Testdaten erfolgreich generiert!")
    print("\n📈 Zusammenfassung:")
    print(f"  👥 Benutzer:        {NUM_USERS}")
    print(f"  🧬 Patienten:       {NUM_PATIENTS}")
    print(f"  🧪 Proben:          {NUM_SAMPLES}")
    print(f"  📋 Aufträge:        {NUM_ORDERS}")
    print(f"  📊 Ergebnisse:      {result_count}")
    print(f"  📝 Audit-Einträge:  {audit_entries}")
    print()
    print("🌐 Öffnen Sie http://localhost:5173/ um die Daten zu sehen!")

if __name__ == "__main__":
    main()
