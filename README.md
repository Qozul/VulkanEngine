# Vulkan Engine

Just a playground for learning Vulkan and graphics tech, brought together to a real scene for graphics coursework. It is in continual development as I learn new things and develop my skills as both a programmer and developer. See https://youtu.be/U8tEOekRktE for an example of a small scene made in the engine for my university masters graphics coursework.

The system generally works as seen in the diagram below, but is always subject to change as I decide on ways to improve and develop it further.

![Image overview of system](https://qozul.github.io./Data/readme-image.png)

# Some Details

The engine as a whole operates on a couple of principles: the scene data drives the graphics work, and the total number of binding commands should be minimized. The scene is important, as buffers require a size, so knowledge of the scene limits allows dynamic storage buffers such as the mvp buffer to be created. All of the data of the scene are packed in to these buffers, and each renderer type has an index in to its segment of data within the large buffer, which is sent to the appropriate shaders as a specialisation constant. Most shaders in the scene follow the same descriptor and push constant layout to remain pipeline layout compatible, leading to the main descriptor set bind being for dynamic offsets for the current frame.

In its current state, the engine rendering path uses deferred shading, beginning with a shadow pass from the sun's view to generate a shadow map of the overall landscape. Next there is the geometry pass to draw the objects in the scene, the lighting pass to generate the lighting on the g-buffer, and the combine pass to mix them together. Finally, the frame enters a ping-pong between two render passes for post processing, before finally being drawn to the swap chain image for presentation. Different render passes are used so that attachments can be used as textures and be sampled from in later passes.

Textures are added to a descriptor array and use the descriptor indexing extension to access these in the shaders with non-uniform indices, allowing instances drawn from the same draw command to use their gl_InstanceIndex to access their own descriptor index in the storage buffers, and potentially sample a different texture on each instance.
