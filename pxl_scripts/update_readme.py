# Run using python3.

import os
import yaml

if os.getcwd().split("/").pop() == "pixie":
	os.chdir("./pxl_scripts")

def script_list_format(folder, script, desc):
	github_url = "https://github.com/pixie-labs/pixie/tree/main/pxl_scripts"
	return f"- {folder}/[{script}]({github_url}/{folder}/{script}): {desc}"

def get_script_list(directory="./px"):
	scripts = []
	for folder in sorted(os.listdir(directory)):
		path = os.path.join(directory,folder,"manifest.yaml")
		if os.path.exists(path):
			with open(path) as yaml_file:
				yaml_text = yaml.safe_load(yaml_file)
				desc = yaml_text.get('long').strip()
				scripts.append(script_list_format(directory, folder, desc))
	return scripts

header_text = """
<!-- The text in this file is automatically generated by the update_readme.py script. -->
# PXL Scripts Overview

Pixie open sources all of its scripts, which serve as examples of scripting in the PxL language. To learn more about PxL, take a look at our [documentation](https://docs.pixielabs.ai/pxl).
"""

footer_text = """

## Contributing

If you want to contribute a new PxL script, please discuss your idea on a Github [issue](https://github.com/pixie-labs/pixie/issues). Since the scripts are exposed to all community users there is a comprehensive review process.

You can contribute a PxL script by forking our repo, adding a new script then creating a pull request. Once the script is accepted it will automatically deploy once the CI process completes.

To learn in more details, you can review this tutorial on [Contributing PxL Scripts](https://docs.pixielabs.ai/using-pixie/scripts/contributing-pxl-scripts/)
"""

with open("./README.md", "w") as f:
	f.write(header_text)
	for script in get_script_list():
		f.write(script + "\n")
	f.write(footer_text)
