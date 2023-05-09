Import("env")

# Make sure esptool is available if we're in a venv (vscode/ubuntu 23.04+)
env.Execute("$PYTHONEXE -m pip install esptool")