#!/bin/bash
set -e

echo "================================================"
echo "  StreamForge — EC2 Deploy Script"
echo "================================================"

# 1. System deps
echo "[1/6] Installing system dependencies..."
sudo dnf update -y -q
sudo dnf install -y gcc-c++ cmake git tmux vim openssl-devel

# 2. Swap (needed for AWS SDK compile on t2.micro)
echo "[2/6] Setting up swap..."
if ! swapon --show | grep -q /swapfile; then
    sudo dd if=/dev/zero of=/swapfile bs=128M count=16 2>/dev/null
    sudo chmod 600 /swapfile
    sudo mkswap /swapfile -q
    sudo swapon /swapfile
    echo "Swap enabled"
else
    echo "Swap already active"
fi

# 3. AWS SDK
echo "[3/6] Building AWS SDK (this takes ~20 min on t2.micro)..."
if [ ! -f /usr/local/lib64/libaws-cpp-sdk-core.so ]; then
    git clone --depth=1 --recurse-submodules --shallow-submodules \
        -b main https://github.com/aws/aws-sdk-cpp.git ~/aws-sdk-cpp
    mkdir -p ~/aws-sdk-cpp/build && cd ~/aws-sdk-cpp/build
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DBUILD_ONLY="s3;dynamodb;sns" \
             -DENABLE_TESTING=OFF \
             -DAUTORUN_UNIT_TESTS=OFF \
             -DCMAKE_INSTALL_PREFIX=/usr/local
    make -j1 && sudo make install
    echo '/usr/local/lib64' | sudo tee /etc/ld.so.conf.d/aws-sdk.conf
    sudo ldconfig
    cd ~
else
    echo "AWS SDK already installed — skipping"
fi

# 4. Build StreamForge
echo "[4/6] Building StreamForge..."
cd ~/streamforge
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local
make streamforge -j1
echo "Build complete"

# 5. systemd service
echo "[5/6] Installing systemd service..."
sudo tee /etc/systemd/system/streamforge.service > /dev/null << 'SERVICE'
[Unit]
Description=StreamForge Real-Time Analytics Pipeline
After=network.target

[Service]
Type=simple
User=ec2-user
WorkingDirectory=/home/ec2-user/streamforge/build
ExecStart=/home/ec2-user/streamforge/build/streamforge
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
SERVICE

sudo systemctl daemon-reload
sudo systemctl enable streamforge
sudo systemctl start streamforge
echo "StreamForge service started"

# 6. Verify
echo "[6/6] Verifying..."
sleep 3
curl -s http://localhost:9090/health && echo ""
echo "================================================"
echo "  StreamForge is live!"
echo "  Ingestion : http://$(curl -s ifconfig.me):8080/ingest"
echo "  Query API : http://$(curl -s ifconfig.me):9090/query"
echo "  Health    : http://$(curl -s ifconfig.me):9090/health"
echo "================================================"
