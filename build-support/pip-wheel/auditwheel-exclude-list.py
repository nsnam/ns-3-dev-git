import os

ns3_path = os.path.dirname(os.path.abspath(os.sep.join([__file__, "../../"])))

for variant in ["lib", "lib64"]:
    lib_dir = os.path.abspath(os.path.join(ns3_path, f"build/{variant}"))
    if not os.path.exists(lib_dir):
        continue
    for lib in os.listdir(lib_dir):
        if "libns3" in lib:
            print(f"--exclude {lib}", end=' ')
