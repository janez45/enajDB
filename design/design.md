# EnajDB

My personal database project, similar to SQLite or maybe a little simpler, with a custom query language.

## Database support

In order of priority:

1. Inserting rows
2. Deleting rows
3. Updating rows
4. Selection and projection (both bag and set)
5. Joins (Inner, Natural, Left, Right, Outer)
6. Basic set operations (Union, Disjunction, etc.)
7. Indexes
8. Schemas
9. Enforcing checks and foreign keys

Single database, single connection for now

## The custom query language

Based on relational calculus and set operations, not so much relational algebra

Logical operators:

```
&& : Logical AND
|| : Logical OR
!: Logical NOT
^: Logical XOR

In order of precedence: NOT, AND, XOR, OR
```

Selection and indicating projections:

```
select t1:Table1(col1, col2, ...), t2:Table2(col3, col4, ...) where P(t1, t2)
select t1:Table1, t2:Table2 where P(t1, t2)
```

Assuming we have the following tables:

```
User (uid int, name string, age int, pop float)
Group (gid string, name string)
Member(uid int, gid string)
```

All groups that Lisa belongs to:

`select g:Group where exists m: Member, u:User . m.gid = g.gid && u.name = 'Lisa' && u.uid = m.uid`

All users in a group with someone named Lisa:

Basic:

```
select u1:User where
    exists (m1, m2): Member, l:User .
        l.name = 'Lisa'
        && m1.gid = m2.gid
        && m1.uid = u1.uid
        && m2.uid = l.uid
```

**TODO: I think this needs some synctactic sugar**

- e.g. intersections betwen columns
- self joins

```
select u1:User where
    exists (m1, m2): Member, l:User .
        l.name = 'Lisa'
        && m1.gid = m2.gid
        && m1.uid = u1.uid
        && m2.uid = l.uid
```

enajDB is super code - like and uses proof-like syntax. it's inspired by the way I used to use mathematical proof syntax to understand SQL and push out queries.
