# EXShadow

A lookdev plugin for Unreal 5.6 that provides an Actor Component that generates a custom shadow map for an Actor that can be accessed in a material.


![Shadow demonstration.](/Images/EXShadowDemo.png)


## Motivation

I have several friends who want to experiment with using shadow maps to drive various material effects (such as blending between different textures based on received shadows).

Shadow maps are not directly accessible to materials when using Unreal’s deferred renderer, they’re written into the light buffer which is an intermediate buffer that’s only accessible in the editor and not in a shipping build. It’s possible to modify the engine code to create a custom buffer to write the shadows to but distributing a custom build of the engine to my friends isn’t a very appealing option.

Luckily Unreal’s USceneCaptureComponent2D has the ability to capture scene depth from it’s view and write it to a render target. SO, I decided to take a stab at using this to generate a custom shadow pass on an ActorComponent that could be distributed as a plugin.

## Usage

Works on Actors that have one or more static/skeletal mesh components.

1. In an Actor Blueprint add an EXShadowActorComponent
2. The component has several properties that can be modified from the Details panel, such as the render target size.
3. Modifiy the mesh materials to include the MF_EXSampleShadowMap Material Function node. The output of this node will be the shadow map.
![MaterialFunction](/Images/EXShadow_Tut.png)
