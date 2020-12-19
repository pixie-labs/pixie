if [[ -n "$(git diff --name-only $TRAVIS_COMMIT_RANGE | grep /px/)" ]]; then
  cp pxl_scripts/README.md pxl_scripts/README_ORIG.md
  ./pxl_scripts/update_readme.py
  if [[ -n "$(diff pxl_scripts/README.md pxl_scripts/README_ORIG.md)" ]]; then
    echo 'ERROR: This diff changes files in the pxl_scripts/px/ directory without updating the pxl_scripts/README.md file. To update the README.md file, run `make update_readme` in the pxl_scripts folder.'
    exit
  fi
fi