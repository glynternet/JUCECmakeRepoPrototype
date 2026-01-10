#!/bin/bash
set -euf -o pipefail

release_version="${1:?must provide version}"
version_file="./Apps/mbk/Source/MainComponent.h"

sed -i "s/\"version: [^\"]*\"/\"version: ${release_version}\"/" "$version_file"
git add "$version_file"
git commit -m "$release_version"
git tag -a "$release_version" -m "$release_version"
