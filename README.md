# WeatherMaestro (Server-only quick start)

This guide covers **cloning**, **building**, and **running** the server on **Linux** or **Windows (via WSL)**.

## Prerequisites
- A C toolchain and Make:
  - **Ubuntu/Debian (incl. WSL):**
    ```bash
    sudo apt update
    sudo apt install -y build-essential
    ```
  - **Fedora:**
    ```bash
    sudo dnf groupinstall -y "Development Tools"
    ```
  - **Arch:**
    ```bash
    sudo pacman -S --needed base-devel
    ```

## Clone the repository (test branch)
```bash
git clone -b test https://github.com/mmandelstrom/WeatherMaestro.git
cd WeatherMaestro
```

## Build the server

> At the moment, only the **server** can be built.

**Option A: Using the root Makefile**
```bash
make server
```

**Option B: Build from the server directory**
```bash
cd server
make
```

## Run the server

### Option A: Using Make
```bash
make server/run
```

### Option B: From server directory
```bash
cd server
./server
```

---

## API documentation
- Swagger (OpenAPI): https://app.swaggerhub.com/apis-docs/chasacademy-178/WeatherMaestro-API/1.0.0
