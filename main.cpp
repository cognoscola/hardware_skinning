#include "main.h"

#include <skybox/skybox.h>
#include <platform/glfw_launcher.h>
#include <mesh/mesh.h>
#include <utils/io/video.h>

int main() {

    Window hardware;             //manage the window
    assert(restart_gl_log());    //restart the log system
    assert(start_gl(&hardware)); //start the glfw instance

    //check that our framebuffer is initialized correctly
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Frame Buffer messed up\n");
    }

    //Assign the input callbacks
    glfwSetCursorPosCallback(hardware.window,cursor_position_callback);
    glfwSetInputMode(hardware.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(hardware.window, key_callback);
    glfwSetInputMode(hardware.window,GLFW_STICKY_KEYS, 1);

    Video video; //image/video capture components
    reserve_video_memory (&video, &hardware);

    camera = {}; //camera components
    cameraInit(&camera, &hardware);

    Mesh monkey;
    meshInit(&monkey, camera.proj_mat);

    Skybox sky; // sky object
    skyInit(&sky, camera.proj_mat);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glEnable (GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);

    //todo
    double anim_time = 0.0;

    while(!glfwWindowShouldClose (hardware.window)) {

        //timing calculation
        static double previous_seconds = glfwGetTime ();
        double current_seconds = glfwGetTime ();
        double elapsed_seconds = current_seconds - previous_seconds;
        previous_seconds = current_seconds;

        anim_time += elapsed_seconds * 0.7;
        if (anim_time >= monkey.animationDuration) {
            anim_time = monkey.animationDuration - anim_time;
        }

        if(videoUpdateTimer(&video, &elapsed_seconds)) break;

        //RENDER TO THE DEFAULT BUFFER
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0,0,hardware.vmode->width,hardware.vmode->height);

        updateMovement(&camera, &input);
        calculateViewMatrices(&camera);
        meshRender(&monkey, &camera, 1000.0f);
        skyRender(&sky, &camera);
        skyUpdate(&sky);

        glfwPollEvents();

        if (GLFW_PRESS == glfwGetKey (hardware.window, GLFW_KEY_P)) {
            video.dump_video = true;
            printf("Recording...");
        }

        if (GLFW_PRESS == glfwGetKey (hardware.window, GLFW_KEY_SPACE)) {
            assert (screencapture (&hardware));
        }
        if (GLFW_PRESS == glfwGetKey(hardware.window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(hardware.window, 1);
        }

        bool monkey_moved = false;
        if (glfwGetKey (hardware.window, 'Z')) {
            monkey.theta += monkey.rot_speed * elapsed_seconds;
            monkey.g_local_anims[0] = rotate_z_deg (identity_mat4 (), monkey.theta);
            monkey.g_local_anims[1] = rotate_z_deg (identity_mat4 (), -monkey.theta);
            monkey_moved = true;
        }

        if (glfwGetKey (hardware.window, 'X')) {
            monkey.theta -= monkey.rot_speed * elapsed_seconds;
            monkey.g_local_anims[0] = rotate_z_deg (identity_mat4 (), monkey.theta);
            monkey.g_local_anims[1] = rotate_z_deg (identity_mat4 (), -monkey.theta);
            monkey_moved = true;
        }

        if (glfwGetKey (hardware.window, 'C')) {
            monkey.y -= 0.5f * elapsed_seconds;
            monkey.g_local_anims[2] = translate (identity_mat4 (), vec3 (0.0f, monkey.y, 0.0f));
            monkey_moved = true;
        }

        if (glfwGetKey (hardware.window, 'V')) {
            monkey.y += 0.5f * elapsed_seconds;
            monkey.g_local_anims[2] = translate (identity_mat4 (), vec3 (0.0f, monkey.y, 0.0f));
            monkey_moved = true;
        }
        meshSkeletonAnimate(
                &monkey,
                monkey.nodes,
                anim_time,
                identity_mat4 (),
                monkey.monkey_bone_offset_matrices,
                monkey.monkey_bone_animation_mats
        );
       glUseProgram (monkey.shader);
        glUniformMatrix4fv (
                monkey.bone_matrices_location[0],
                monkey.boneCount,
                GL_FALSE,
                monkey.monkey_bone_animation_mats[0].m
        );

        if (video.dump_video) { // check if recording mode is enabled
            while (video.video_dump_timer > video.frame_time) {
                grab_video_frame (&video, &hardware); // 25 Hz so grab a frame
                video.video_dump_timer -= video.frame_time;
            }
        }
        glfwSwapBuffers(hardware.window);
    }

    meshCleanUp(&monkey);
    skyCleanUp(&sky);

    if(video.dump_video) {
        dump_video_frames(&video, &hardware);
    }

    /* close GL context and any other GLFW resources */
    glfwTerminate();
    return 0;
}

/**
 * Called when we press a key on the keyboard
 * in window - the focused window
 * in key    - which key?
 * in scancode
 * in action - One of GFLW_PRESS, GLFW_REPEAT or GLFW_RELEASE
 */
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
        case GLFW_KEY_W:
            input.wPressed = action == GLFW_PRESS ? true: ((action == GLFW_RELEASE) ? false: input.wPressed);
            camera.move_angle =  action == GLFW_PRESS ? 0:camera.move_angle;
            break;
        case GLFW_KEY_S:
            input.sPressed = action == GLFW_PRESS ? true: ((action == GLFW_RELEASE) ? false: input.sPressed);
            camera.move_angle =  action == GLFW_PRESS ? 180:camera.move_angle;
            break;
        case GLFW_KEY_A:
            input.aPressed = action == GLFW_PRESS ? true: ((action == GLFW_RELEASE) ? false: input.aPressed);
            camera.move_angle =  action == GLFW_PRESS ? -90:camera.move_angle;
            break;
        case GLFW_KEY_D:
            input.dPressed = action == GLFW_PRESS ? true: ((action == GLFW_RELEASE) ? false: input.dPressed);
            camera.move_angle =  action == GLFW_PRESS ? 90:camera.move_angle;
            break;
    }
}

/**
 * Called every time the cursor moves. It is used to calculate the Camera's direction
 * in window - the window holding the cursor
 * in xpos   - the xposition of the cursor on the screen
 * in ypos   - the yposition of the curose on the screen
 */
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {

    //calculate pitch
    static double  previous_ypos = ypos;
    double position_y_difference = ypos - previous_ypos;
    previous_ypos = ypos;

    //calculate yaw
    static double previous_xpos = xpos;
    double position_x_difference = xpos - previous_xpos;
    previous_xpos = xpos;

    //reduce signal
    camera.yaw += position_x_difference *camera.signal_amplifier;
    camera.pitch += position_y_difference *camera.signal_amplifier;

    //calculate rotation sequence
    calculateRotationMatrix(camera.pitch, &camera.Rpitch, PITCH);
    calculateRotationMatrix(camera.yaw, &camera.Ryaw, YAW);
}