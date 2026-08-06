- What if you kill the program using SIGTERM?

GOAL: Single user connection, single database, multiple tables, index support, query plans

NOW
│
├── [1] Refactor Pager (DONE, NEED PAGER TESTS)
│ ├── PageId
│ ├── Page
│ ├── open()
│ ├── get_page()
│ ├── allocate_page()
│ ├── flush()
│ └── persistence tests - No memory leak

- PAGER IS DONE!!
  │
  ├── [2] Replace fixed Row
  │ ├── Type
  │ ├── Value
  │ ├── Column
  │ ├── Schema
  │ └── Row
  │
  - Table tests
    - Attempting to read/write on a nonexistent row
    - Attempting to get an invalid page
    - Correct file size
    - Reading and Writing across pages

  ├── [3] Generic serialization
  │
  ├── [4] In-memory B+ tree
  │
  ├── [5] Persistent B+ tree
  │
  └── [6] THEN introduce Database + Catalog
  │
  ├── CREATE TABLE
  ├── DROP TABLE
  ├── OPEN TABLE
  ├── multiple schemas
  └── multiple B+ tree roots

What will make enajDB unique?

- PAX
- Custom proof-like language, a little more like what I used to use for understanding SQL for the first time
