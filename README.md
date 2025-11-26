# EXShadow

A lookdev plugin for Unreal 5.6 that provides an Actor Component that generates a custom shadow map for an Actor that can be accessed in a material.


![Shadow demonstration.](/Images/EXShadowDemo.png)


## Motivation

I have several friends who want to experiment with using shadow maps to drive various material effects (such as blending between different textures based on received shadows).

Shadow maps are not directly accessible to materials when using Unreal’s deferred renderer, they’re written into the light buffer which is an intermediate buffer that’s only accessible in the editor and not in a shipping build. It’s possible to modify the engine code to create a custom buffer to write the shadows to but distributing a custom build of the engine to my friends isn’t a very appealing option.

Luckily Unreal’s USceneCaptureComponent2D has the ability to capture scene depth from it’s view and write it to a render target. SO, I decided to take a stab at using this to generate a custom shadow pass on an ActorComponent that could be distributed as a plugin.

## Usage

Works on Actors that have one or more static/skeletal mesh components.
The intent is for it to be used with Unlit materials (which will not receive default shadows) where you build your own shading model.

1. In an Actor Blueprint add an EXShadowActorComponent
2. The component has several properties that can be modified from the Details panel, such as the render target size.
3. Modify the mesh materials to include the MF_EXSampleShadowMap Material Function node. The output of this node will be the shadow map.
![MaterialFunction](/Images/EXShadow_Tut.png)

## Issues and Limitations

This is intended to be a lookdev tool, not something that should be used in a shipping title due to it not being very efficient as is:
* Each instance of an actor that uses this component will generate it's own shadow depth render target.
* Because the shadow map generation is happening inside a material there are transformations that are happening on a per-pixel basis that should really be happening per-vertex.

## Future Work

I recently learned (thanks to William Mishra-Manning's recent blog post https://medium.com/@manning.w27/advanced-graphics-programming-in-unreal-part-1-10488f2e17dd) that it's possible to add custom rendering passes from inside a plugin without having to modify the engine.
I've been learning Unreal's graphics programming architecture and would like to attempt to do a proper custom shadow rendering pass. 
