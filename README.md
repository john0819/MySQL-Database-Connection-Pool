# MySQL-Database-Connection-Pool
Developed a C++ based MySQL database connection pool to improve the access efficiency of MySQL Server in high concurrency situations

## Project Overview
This project aims to enhance the efficiency of MySQL database access under high concurrency by developing a C++ based MySQL database connection pool. By reducing the frequent TCP connection setups and MySQL authentication processes, significant performance improvements are achieved.

![image](https://github.com/john0819/MySQL-Database-Connection-Pool/assets/70586660/5d3f8a79-a521-4491-bf38-10cdbed22e35)

## Key Technologies
- **Database Programming**: Utilized MySQL's C++ client library to encapsulate database connections and operations.
- **Design Pattern**: Implemented the Singleton pattern to ensure the connection pool's uniqueness and global access.
- **Data Structures**: Managed database connections using a queue container.
- **Multithreading**: Leveraged C++11 multithreading features, including mutexes and condition variables, to ensure thread-safe operations within the connection pool.
- **Smart Pointers and Lambda Expressions**: Employed shared_ptr and lambda expressions to manage connection lifecycles, enhancing code readability and maintainability.
- **Producer-Consumer Model**: Adopted the producer-consumer threading model to ensure efficient allocation and recycling of connections.

## Project Background
To alleviate the access bottleneck of MySQL databases (based on the C/S design), adding a connection pool can improve the efficiency of MySQL Server access. Under high concurrency, the performance costs of frequent TCP handshakes, MySQL Server connection authentication, and resource recovery from connection closures are significant. Increasing the connection pool reduces these performance losses.

## Connection Pool Features
- **Initial Connection Size (initSize)**: Creates a certain number of connections during initialization to reduce the overhead of connection creation on the first access.
- **Maximum Connection Size (maxSize)**: Dynamically increases the number of connections to handle high concurrency requests, but does not exceed the maximum connection size to prevent excessive resource usage.
- **Maximum Idle Time (maxIdleTime)**: Connections that have not been used within the specified idle time will be released, maintaining a reasonable number of connections in the pool.
- **Connection Timeout (connectionTimeout)**: Specifies the wait time to acquire a connection when all connections are in use. If the wait time exceeds the timeout, the connection acquisition fails.

## Performance Testing
Stress test results show that for 5000 data insert operations, the single-threaded operation time using the connection pool was reduced by 46.4%, and the multi-threaded operation time was reduced by 13.5%. Detailed results are as follows:

| Data Size | No Pool (Single Thread) | No Pool (Four Threads) | With Pool (Single Thread) | With Pool (Four Threads) |
|-----------|-------------------------|------------------------|--------------------------|--------------------------|
| 1000      | 1891ms                  | 497ms                  | 1079ms                   | 408ms                    |
| 5000      | 10033ms                 | 2361ms                 | 5380ms                   | 2041ms                   |
| 10000     | 19403ms                 | 4589ms                 | 10522ms                  | 4034ms                   |

## Code Structure
- `ConnectionPool.cpp` and `ConnectionPool.h`: Connection pool implementation.
- `Connection.cpp` and `Connection.h`: Database operation code, including CRUD operations.

## Usage
Include the `ConnectionPool` module in your project and obtain database connections by calling `ConnectionPool::getInstance()->getConnection()`. Connections are automatically returned to the pool after use.

## Contribution
Suggestions for improvements and code contributions are welcome. Please submit Pull Requests or Issues.

## License
This project is licensed under the MIT License. See the LICENSE file for details.
