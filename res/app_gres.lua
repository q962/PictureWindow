function run(target)
    os.execv("sass", {"--no-source-map", "main.scss", "main.css"}, {
        curdir = os.projectdir() .. "/res/css"
    })
end
