from dotenv import load_dotenv
import os
import psycopg2

load_dotenv()
user = os.getenv("PG_ROOT_USER")
password = os.getenv("PG_ROOT_PASSWORD")
db = os.getenv("PG_DB")


def connect_to_db(user=user, password=password, db=db):
    conn = psycopg2.connect(user=user, password=password, database=db)
    cur = conn.cursor()
    conn.autocommit = True
    return conn, cur
