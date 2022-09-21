Import("env")
env.AddCustomTarget(
    "uploadnobuild", 
    None, 
    'pio run -e %s -t nobuild -t upload -t monitor' %
        env["PIOENV"], 
    title="Upload without building"
)