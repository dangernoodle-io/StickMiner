Import("env")

env.Append(
    CCFLAGS=["--coverage"],
    LINKFLAGS=["--coverage"]
)
