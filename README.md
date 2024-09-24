# OpenGL Order-Independent Transparency

This project contains the renderers of my bachelor thesis: https://www.researchgate.net/publication/332371662_Order-Independent_Transparency_Acceleration

![image](https://github.com/user-attachments/assets/4105e930-dd7f-4305-9279-f6284edc9018)


# Implemented Renderers

Use the console to change the active renderer:

`renderer = dynamic_fragment`: This renderer implements the dynamic fragment buffer to handle transparency: https://doi.org/10.1109/SIBGRAPI.2012.27 (Memory-Efficient Order-Independent Transparency with Dynamic Fragment Buffer)

`renderer = forward`: This renderer is a simple forward renderer that uses alpha blending to draw transparency. However, it **does not sort** the geometry, therefore the result will be wrong.

`renderer = linked`: This renderer uses a linked list to store transparent fragments. http://dx.doi.org/10.1111/j.1467-8659.2010.01725.x (Real-Time Concurrent Linked List Construction on the GPU). However, in contrast to the paper, my implementation only stores depth and alpha pairs (without colors) and does not use sorting, but requires a second render pass of transparent objects instead.

`renderer = multilayer_alpha4`: This renderer uses multi-layer alpha blending with 4 nodes per pixel. It is possible to use any two digit number to specify the number of nodes. https://dl.acm.org/doi/10.1145/2556700.2556705 (Multi-layer alpha blending)

`renderer = adaptive4`: This renderer implements adaptive transparency with 4 nodes. Again, any two digit number is possible to specify the number of nodes. https://dl.acm.org/doi/10.1145/2018323.2018342 (Adaptive transparency)

`renderer = weighted_oit`: This renderer implements weighted OIT. https://jcgt.org/published/0002/02/09/ (Weighted Blended Order-Independent Transparency)

# Personal Recommendations

For the best results use either `dynamic_fragment` or `linked`. Dynamic Fragment should be a little bit faster, but is also more difficult to implement. Both methods require two render passes of the transparent geometry.

If you need a fast solution and don't really care about the realism of the transparency, weighted OIT is a fast solution, but it might need some scene-dependent tuning of the depth function inside the fragment shader.

If you need a fast and fairly accurate version, I would reccomend multi-layer alpha blending with 4 nodes. However, our implementation uses a spin lock in the fragment shader that does not work for all graphic vendors. There is am opengl extension that can be used as a workaround: `GL_ARB_fragment_shader_interlock` is a specification to replace the spinlock with a proper critical section. 

I would not recommend adaptive transparency, since the results of multi-layer alpha blending are generally surperior.
