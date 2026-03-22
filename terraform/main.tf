terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.region
}

variable "region"        { default = "ap-south-1" }
variable "instance_count"{ default = 3 }
variable "instance_type" { default = "t2.micro" }
variable "s3_bucket"     { default = "streamforge-events-adarsh" }
variable "sns_arn"       { default = "" }

# Security group
resource "aws_security_group" "streamforge" {
  name        = "streamforge-sg"
  description = "StreamForge cluster security group"

  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  ingress {
    from_port   = 8080
    to_port     = 8080
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  ingress {
    from_port   = 9090
    to_port     = 9090
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  ingress {
    from_port   = 8090
    to_port     = 8090
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

# EC2 instances
resource "aws_instance" "streamforge" {
  count         = var.instance_count
  ami           = "ami-0f58b397bc5c1f2e8"
  instance_type = var.instance_type

  vpc_security_group_ids = [aws_security_group.streamforge.id]

  iam_instance_profile = aws_iam_instance_profile.streamforge.name

  user_data = <<-USERDATA
    #!/bin/bash
    yum update -y
    yum install -y git cmake gcc-c++ openssl-devel libcurl-devel
    cd /home/ec2-user
    git clone https://github.com/Adarsh73111/Streamforge.git
    cd Streamforge
    bash scripts/deploy_ec2.sh
  USERDATA

  tags = {
    Name    = "streamforge-node-${count.index + 1}"
    Project = "StreamForge"
    Version = "v3.0"
  }
}

# IAM role
resource "aws_iam_role" "streamforge" {
  name = "streamforge-ec2-role"
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action    = "sts:AssumeRole"
      Effect    = "Allow"
      Principal = { Service = "ec2.amazonaws.com" }
    }]
  })
}

resource "aws_iam_role_policy_attachment" "s3" {
  role       = aws_iam_role.streamforge.name
  policy_arn = "arn:aws:iam::aws:policy/AmazonS3FullAccess"
}

resource "aws_iam_role_policy_attachment" "dynamo" {
  role       = aws_iam_role.streamforge.name
  policy_arn = "arn:aws:iam::aws:policy/AmazonDynamoDBFullAccess"
}

resource "aws_iam_role_policy_attachment" "sns" {
  role       = aws_iam_role.streamforge.name
  policy_arn = "arn:aws:iam::aws:policy/AmazonSNSFullAccess"
}

resource "aws_iam_instance_profile" "streamforge" {
  name = "streamforge-instance-profile"
  role = aws_iam_role.streamforge.name
}

# Application Load Balancer
resource "aws_lb" "streamforge" {
  name               = "streamforge-alb"
  internal           = false
  load_balancer_type = "application"
  security_groups    = [aws_security_group.streamforge.id]
}

resource "aws_lb_target_group" "streamforge" {
  name     = "streamforge-tg"
  port     = 8080
  protocol = "HTTP"

  health_check {
    path                = "/health"
    port                = "8080"
    healthy_threshold   = 2
    unhealthy_threshold = 3
    interval            = 30
  }
}

resource "aws_lb_listener" "streamforge" {
  load_balancer_arn = aws_lb.streamforge.arn
  port              = 80
  protocol          = "HTTP"
  default_action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.streamforge.arn
  }
}

resource "aws_lb_target_group_attachment" "streamforge" {
  count            = var.instance_count
  target_group_arn = aws_lb_target_group.streamforge.arn
  target_id        = aws_instance.streamforge[count.index].id
  port             = 8080
}

output "load_balancer_dns" {
  value       = aws_lb.streamforge.dns_name
  description = "StreamForge Load Balancer URL"
}

output "node_ips" {
  value       = aws_instance.streamforge[*].public_ip
  description = "StreamForge Node IPs"
}
