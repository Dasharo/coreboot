#!/bin/bash

file=$1

sha256sum $file > $file.sha256
gpg --default-key 5DC481E1F371151E --armor --output $file.sha256.sig --detach-sig $file.sha256
gpg --verify $file.sha256.sig $file.sha256
