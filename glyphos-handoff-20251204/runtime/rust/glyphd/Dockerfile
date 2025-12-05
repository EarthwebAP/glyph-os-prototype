# Multi-stage Dockerfile for glyphd

FROM rust:1.75 as builder

WORKDIR /usr/src/glyph-os

# Copy workspace files
COPY Cargo.toml ./
COPY glyphd/Cargo.toml ./glyphd/
COPY glyph-spu/Cargo.toml ./glyph-spu/

# Create dummy source files for dependency caching
RUN mkdir -p glyphd/src && \
    echo "fn main() {}" > glyphd/src/main.rs && \
    mkdir -p glyph-spu/src && \
    echo "fn main() {}" > glyph-spu/src/main.rs

# Build dependencies
RUN cargo build --release --bin glyphd

# Remove dummy source
RUN rm -rf glyphd/src

# Copy actual source
COPY glyphd/src ./glyphd/src

# Build for release
RUN cargo build --release --bin glyphd

# Runtime stage
FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y ca-certificates curl && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/src/glyph-os/target/release/glyphd /usr/local/bin/glyphd

EXPOSE 8080

ENV RUST_LOG=info

CMD ["glyphd"]
