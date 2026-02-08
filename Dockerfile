# Use lightweight Linux
FROM ubuntu:22.04

# Install build tools and runtime deps
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy all project files
COPY . .

# Ensure server is executable (important on Linux)
RUN chmod +x server || true

# Compile backend
RUN gcc server.c backend.c -o server -lm -lpthread

# Expose backend port
EXPOSE 8080

# Start the server
CMD ["./server"]
