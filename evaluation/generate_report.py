# generate_report.py
import sys
import os
import cv2
from datetime import datetime

def diffImage(filename, refPath):
    try:
        img1 = cv2.imread(filename)
        img2 = cv2.imread(refPath)
        print(img1.shape)
        print(img2.shape)
        diff = cv2.absdiff(img1, img2)
        cv2.imwrite("diff_" + filename, diff)
    except Exception as e:
        print(e)
        file.write("\\\\Could not compute difference image, exception message see job log.\n")

def printImages(file, filename, refPath):
    if os.path.isfile(filename) and os.path.isfile("diff_" + filename):
        file.write("\\begin{figure}[H]\n\\centering\n\\includegraphics[width=0.3\\textwidth]{" + filename + "}\n\\includegraphics[width=0.3\\textwidth]{" + refPath + "}\n\\includegraphics[width=0.3\\textwidth]{diff_" + filename + "}\n\\end{figure}\n")
        return
    file.write("\\\\Error: Some of the necessary files do not exist.\n")
    if not os.path.isfile(filename) and os.path.isfile("diff_" + filename):
        file.write("\\begin{figure}[H]\n\\centering\n\\includegraphics[width=0.3\\textwidth]{owl.png}\n\\includegraphics[width=0.3\\textwidth]{" + refPath + "}\n\\includegraphics[width=0.3\\textwidth]{diff_" + filename + "}\n\\end{figure}\n")
    elif os.path.isfile(filename) and not os.path.isfile("diff_" + filename):
        file.write("\\begin{figure}[H]\n\\centering\n\\includegraphics[width=0.3\\textwidth]{" + filename + "}\n\\includegraphics[width=0.3\\textwidth]{" + refPath + "}\n\\includegraphics[width=0.3\\textwidth]{owl.png}\n\\end{figure}\n")
    elif not os.path.isfile(filename) and not os.path.isfile("diff_" + filename):
        file.write("\\begin{figure}[H]\n\\centering\n\\includegraphics[width=0.3\\textwidth]{owl.png}\n\\includegraphics[width=0.3\\textwidth]{" + refPath + "}\n\\includegraphics[width=0.3\\textwidth]{owl.png}\n\\end{figure}\n")

if __name__ == "__main__":
    tasks_dict = {
        "task1": 1,
        "task2": 2,
        "task3": 3,
        "task4": 4,
        "task5": 5,
        "task6": 6,
        "boni2": 2,
        "boni4": 4,
        "boni5": 5,
        "boni6": 6
    }
    task_names = ["Task 1", "Task 2", "Task 3", "Task 4", "Task 5", "Task 6"]
    deadlines = [
        "2024-10-18T23:55:00+02:00",
        "2024-11-08T23:55:00+02:00",
        "2024-11-22T23:55:00+02:00",
        "2024-12-06T23:55:00+02:00",
        "2025-01-10T23:55:00+02:00",
        "2025-01-24T23:55:00+02:00"
    ]

    task = tasks_dict[sys.argv[1]]
    task_idx = task - 1
    filenamePrefix = ""
    if "boni" in sys.argv[1]:
        filenamePrefix = "bonus_"

    vkExists = os.path.exists("../_project/GCGProject_VK")
    glExists = os.path.exists("../_project/GCGProject_GL")
    print("Using Vulkan: " + str(vkExists))
    print("Using OpenGL: " + str(glExists))

    pathPrefix = ""
    if glExists:
        pathPrefix = "GCG_GL/"
    if vkExists:
        pathPrefix = "GCG_VK/"

    cameraPoses = ["front"]
    if task > 1:
        cameraPoses = ["front", "front_right", "right", "front_left", "left", "front_up", "up", "front_down", "down", "right_up", "right_down", "left_up", "left_down", "back", "back_left", "back_right", "front_left_up", "front_right_up"]

    with open("report.tex","w") as file:
        file.write("\\documentclass{article}\n")
        file.write("\\usepackage{graphicx}\n")
        file.write("\\usepackage{subcaption}\n")
        file.write("\\usepackage{float}\n")
        file.write("\\usepackage[a4paper, margin=1in]{geometry}\n")
        file.write("\\title{" + task_names[task_idx] + " Report}\n")
        file.write("\\begin{document}\n")
        file.write("\\maketitle\n")

        timestamp_str = sys.argv[2]
        file.write("Timestamp: " + timestamp_str + "\n")
        deadline_str = deadlines[task_idx]
        file.write("\\\\Deadline: " + deadline_str + "\n")

        try:
            # Parse the timestamps into datetime objects
            timestamp = datetime.strptime(timestamp_str, "%Y-%m-%dT%H:%M:%S+02:00")
            deadline = datetime.strptime(deadline_str, "%Y-%m-%dT%H:%M:%S+02:00")

            # Compare the timestamps
            if timestamp < deadline:
                file.write("\\\\The timestamp has not passed the deadline.\n")
                time_remaining = deadline - timestamp
                file.write(f"\\\\Time remaining: {time_remaining.total_seconds() / 3600:.2f} hours.\n")
            else:
                file.write("\\\\The timestamp has passed the deadline.\n")
                time_passed = timestamp - deadline
                file.write(f"\\\\Time passed: {time_passed.total_seconds() / 3600:.2f} hours.\n")
        except Exception as e:
            print(e)
            file.write("\\\\Could not compute difference to deadline, exception message see job log.\n")

        file.write("\\section{Results}\n")
        file.write("Side-by-side comparisons: left = your solution, middle = reference image, right = absolute difference.\n")

        if not vkExists and not glExists:
            file.write("\\\\We could not decide whether your taking the OpenGL or Vulkan Route. Please stick to the original folder names.\n")
    
        file.write("\\subsection{Standard View}\n")
        for cameraPos in cameraPoses:
            filename = "standard_" + cameraPos + ".png"
            refPath = pathPrefix + filenamePrefix + filename
            diffImage(filename, refPath)
            printImages(file, filename, refPath)
        file.write("\\newpage\n")

        if task >= 3:
            file.write("\\subsection{Backface Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")
        
        if task == 3 or task == 4:
            file.write("\\subsection{Wireframe View}\n")
            for cameraPos in cameraPoses:
                filename = "wireframe_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")

            file.write("\\subsection{Wireframe and Backframe Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_wireframe_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")

        if task == 5:
            file.write("\\subsection{Normals View}\n")
            for cameraPos in cameraPoses:
                filename = "normals_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")

            file.write("\\subsection{Normals Backface Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_normals_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")

        if task == 6:
            file.write("\\subsection{Texcoords View}\n")
            for cameraPos in cameraPoses:
                filename = "texcoords_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")

            file.write("\\subsection{Texcoords Backface Culling View}\n")
            for cameraPos in cameraPoses:
                filename = "culling_texcoords_" + cameraPos + ".png"
                refPath = pathPrefix + filenamePrefix + filename
                diffImage(filename, refPath)
                printImages(file, filename, refPath)
            file.write("\\newpage\n")

        file.write("\\newpage\n")
        file.write("\\end{document}\n")
