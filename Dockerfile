# OpenSylab Backend Dockerfile
# Multi-stage build for optimized image size

# Stage 1: Build stage
FROM gcc:13-bookworm AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    libsqlite3-dev \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy CMake files and source code
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY test/ ./test/

# Build the application
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Stage 2: Runtime stage
FROM debian:bookworm-slim

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create app user for security
RUN useradd -m -u 1000 -s /bin/bash opensylab

# Set working directory
WORKDIR /app

# Copy built binary from builder stage
COPY --from=builder /app/build/bin/OpenSylab /app/OpenSylab
COPY --from=builder /app/build/bin/opensylab_tests /app/opensylab_tests

# Create data directory for SQLite database
RUN mkdir -p /app/data && chown -R opensylab:opensylab /app

# Switch to non-root user
USER opensylab

# Expose API port
EXPOSE 8080

# Set environment variables
ENV OPENSYLAB_DB_PATH=/app/data/opensylab.db
ENV OPENSYLAB_API_PORT=8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/api/v1/health || exit 1

# Run the application
CMD ["/app/OpenSylab", "--api", "--api-port", "8080"]
