#!/bin/bash
#update pacman
pacman -Sy
pacman -S bash pacman msys2-runtime --noconfirm --needed
pacman -Su --noconfirm
