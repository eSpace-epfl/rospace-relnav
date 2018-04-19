
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2017 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// appleseed.renderer headers. Only include header files from renderer/api/.
#include "renderer/api/bsdf.h"
#include "renderer/api/edf.h"
#include "renderer/api/camera.h"
#include "renderer/api/color.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/frame.h"
#include "renderer/api/light.h"
#include "renderer/api/log.h"
#include "renderer/api/material.h"
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"
#include "renderer/api/scene.h"
#include "renderer/api/surfaceshader.h"
#include "renderer/api/utility.h"

//inotify stuff
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

//json stuff
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"
#include "foundation/math/matrix.h"
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/log/consolelogtarget.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/searchpaths.h"

// Standard headers.
#include <cstddef>
#include <memory>
#include <string>
	
// Define shorter namespaces for convenience.
namespace asf = foundation;
namespace asr = renderer;

asf::auto_release_ptr<asr::Project> build_project(float* sat_pose, float* sat_pos)
{
 
asf::Quaterniond quat_pose;
    quat_pose.s = sat_pose[3];
    quat_pose.v[0] = sat_pose[0];
    quat_pose.v[1] = sat_pose[1];
    quat_pose.v[2] = sat_pose[2];
    // Create an empty project.
    asf::auto_release_ptr<asr::Project> project(asr::ProjectFactory::create("test project"));
    project->search_paths().push_back("/home/pantic/GIT/appleseed/appleseed/sandbox/samples/cpp/basic/data/");

    // Add default configurations to the project.
    project->add_default_configurations();

    // Set the number of samples. This is the main quality parameter: the higher the
    // number of samples, the smoother the image but the longer the rendering time.
    project->configurations()
        .get_by_name("final")->get_parameters()
            .insert_path("uniform_pixel_renderer.samples", "32");

    // Create a scene.
    asf::auto_release_ptr<asr::Scene> scene(asr::SceneFactory::create());

    // Create an assembly.
    asf::auto_release_ptr<asr::Assembly> assembly(
        asr::AssemblyFactory().create(
            "assembly",
            asr::ParamArray()));

    //------------------------------------------------------------------------
    // Materials
    //------------------------------------------------------------------------

    // Create a color called "gray" and insert it into the assembly.

    static const float RedReflectance[] = { 1.0f, 0.0f, 0.0f };
    assembly->colors().insert(
        asr::ColorEntityFactory::create(
            "red",
            asr::ParamArray()
                .insert("color_space", "srgb"),
            asr::ColorValueArray(3, RedReflectance)));

    static const float GrayReflectance[] = { 0.5f, 0.5f, 0.5f };
    assembly->colors().insert(
        asr::ColorEntityFactory::create(
            "gray",
            asr::ParamArray()
                .insert("color_space", "srgb"),
            asr::ColorValueArray(3, GrayReflectance)));

   static const float WhiteReflectance[] = { 1.0f, 1.0f, 1.0f };
    assembly->colors().insert(
        asr::ColorEntityFactory::create(
            "white",
            asr::ParamArray()
                .insert("color_space", "srgb")
		.insert("multiplier", "110000.0"),
            asr::ColorValueArray(3, WhiteReflectance)));

    // Create a BRDF called "diffuse_gray_brdf" and insert it into the assembly.
    assembly->bsdfs().insert(
        asr::GlossyBRDFFactory().create(
            "diffuse_gray_brdf",
            asr::ParamArray()
                .insert("reflectance", "0.25")
		.insert("ior", "0.9034")
		.insert("roughness", "0.43")
		.insert("fresnel_weight", "0.9")
		.insert("mdf", "ggx")));

// Create a EDF called "bla" and insert it into the assembly.
    assembly->edfs().insert(
        asr::DiffuseEDFFactory().create(
            "diffuse_white_edf",
            asr::ParamArray()
                .insert("radiance", "white")));


// Create a EDF called "bla" and insert it into the assembly.
    assembly->edfs().insert(
        asr::DiffuseEDFFactory().create(
            "diffuse_red_edf",
            asr::ParamArray()
                .insert("radiance", "red")));


  assembly->bsdfs().insert(
        asr::LambertianBRDFFactory().create(
            "diffuse_white_brdf",
            asr::ParamArray()
                .insert("reflectance", "white")));

    // Create a physical surface shader and insert it into the assembly.
    assembly->surface_shaders().insert(
        asr::PhysicalSurfaceShaderFactory().create(
            "physical_surface_shader",
            asr::ParamArray()));

    // Create a material called "gray_material" and insert it into the assembly.
    assembly->materials().insert(
        asr::GenericMaterialFactory().create(
            "gray_material",
            asr::ParamArray()
                .insert("surface_shader", "physical_surface_shader")
                .insert("bsdf", "diffuse_gray_brdf")));



    // Create a material called "gray_material" and insert it into the assembly.
    assembly->materials().insert(
        asr::GenericMaterialFactory().create(
            "light_material",
            asr::ParamArray()
                .insert("surface_shader", "physical_surface_shader")
                .insert("bsdf", "diffuse_white_brdf")
                .insert("edf", "diffuse_white_edf")));

    //------------------------------------------------------------------------
    // Geometry
    //------------------------------------------------------------------------

    // Load the scene geometry from disk.
    asr::MeshObjectArray objects2;
    asr::MeshObjectReader::read(
        project->search_paths(),
        "sun",
        asr::ParamArray()
            .insert("filename", "sun.obj"),
        objects2);

    // Insert all the objects into the assembly.
    for (size_t i = 0; i < objects2.size(); ++i)
    {
        // Insert this object into the scene.
        asr::MeshObject* object = objects2[i];
        assembly->objects().insert(asf::auto_release_ptr<asr::Object>(object));
  const asf::Transformd ExpectedTransform(	            
asf::Transformd::from_local_to_parent(asf::Matrix4d::make_translation(asf::Vector3d(-1.49e10,  0,5e5))));


        // Create an instance of this object and insert it into the assembly.
        const std::string instance_name = std::string(object->get_name()) + "_inst";
       assembly->object_instances().insert(
            asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                asr::ParamArray(),
                object->get_name(),
                ExpectedTransform,
                asf::StringDictionary()
                    .insert("sun2", "light_material")
                    .insert("sun0", "light_material")));


    }

    // Load the scene geometry from disk.
    asr::MeshObjectArray objects;
    asr::MeshObjectReader::read(
        project->search_paths(),
        "cube",
        asr::ParamArray()
            .insert("filename", "scene.obj"),
        objects);

    // Insert all the objects into the assembly.
    for (size_t i = 0; i < objects.size(); ++i)
    {
    // Insert this object into the scene.
        asr::MeshObject* object = objects[i];
        assembly->objects().insert(asf::auto_release_ptr<asr::Object>(object));


    asf::Quaterniond quat_pose;
    quat_pose.s = sat_pose[3];
    quat_pose.v[0] = sat_pose[0];
    quat_pose.v[1] = sat_pose[1];
    quat_pose.v[2] = sat_pose[2];



  const asf::Transformd ExpectedTransform(	                        
asf::Transformd::from_local_to_parent(asf::Matrix4d::make_translation(asf::Vector3d(sat_pos[0], sat_pos[1],sat_pos[2]))*asf::Matrix4d::make_rotation(quat_pose)));

     

 // Create an instance of this object and insert it into the assembly.
        const std::string instance_name = "teeest";
         asf::auto_release_ptr<asr::ObjectInstance> obj = asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                asr::ParamArray(),
                object->get_name(),
                ExpectedTransform,
                asf::StringDictionary()
                    .insert("default", "gray_material")
                    .insert("default2", "gray_material"));

        assembly->object_instances().insert(obj);
           
    }

    //------------------------------------------------------------------------
    // Light
    //------------------------------------------------------------------------

    // Create a color called "light_intensity" and insert it into the assembly.
      static const float SunRadianceValues[31] =
            {
                21127.5f, 25888.2f, 25829.1f, 24232.3f,
                26760.5f, 29658.3f, 30545.4f, 30057.5f,
                30663.7f, 28830.4f, 28712.1f, 27825.0f,
                27100.6f, 27233.6f, 26361.3f, 25503.8f,
                25060.2f, 25311.6f, 25355.9f, 25134.2f,
                24631.5f, 24173.2f, 23685.3f, 23212.1f,
                22827.7f, 22339.8f, 21970.2f, 21526.7f,
                21097.9f, 20728.3f, 20240.4f
            };

    assembly->colors().insert(
        asr::ColorEntityFactory::create(
            "light_intensity",
            asr::ParamArray()
                .insert("color_space", "spectral")
                .insert("multiplier", "1000000000000000000000.0"),
            asr::ColorValueArray(31, SunRadianceValues)));

    // Create a point light called "light" and insert it into the assembly.
    asf::auto_release_ptr<asr::Light> light(
        asr::SunLightFactory().create(
            "light",
            asr::ParamArray()
                .insert("turbidity", "0")
		.insert("radiance_multiplier", "151651651651000000000006566")));

    //assembly->lights().insert(light);

    //------------------------------------------------------------------------
    // Assembly instance
    //------------------------------------------------------------------------

    // Create an instance of the assembly and insert it into the scene.
    asf::auto_release_ptr<asr::AssemblyInstance> assembly_instance(
        asr::AssemblyInstanceFactory::create(
            "assembly_inst",
            asr::ParamArray(),
            "assembly"));
    assembly_instance
        ->transform_sequence()
            .set_transform(
                0.0,
                asf::Transformd::identity());
    scene->assembly_instances().insert(assembly_instance);

    // Insert the assembly into the scene.
    scene->assemblies().insert(assembly);

    //------------------------------------------------------------------------
    // Environment
    //------------------------------------------------------------------------

    // Create a color called "sky_radiance" and insert it into the scene.
    static const float SkyRadiance[] = { 0.0f, 0.0f, 0.0f };
    scene->colors().insert(
       asr::ColorEntityFactory::create(
          "sky_radiance",
            asr::ParamArray()
                .insert("color_space", "srgb")
                .insert("multiplier", "0.0"),
            asr::ColorValueArray(3, SkyRadiance)));

    // Create an environment EDF called "sky_edf" and insert it into the scene.
    scene->environment_edfs().insert(
        asr::ConstantEnvironmentEDFFactory().create(
            "sky_edf",
            asr::ParamArray()
                .insert("radiance", "sky_radiance")));

    // Create an environment shader called "sky_shader" and insert it into the scene.
    scene->environment_shaders().insert(
        asr::EDFEnvironmentShaderFactory().create(
            "sky_shader",
            asr::ParamArray()
                .insert("environment_edf", "sky_edf")));

    // Create an environment called "sky" and bind it to the scene.
   scene->set_environment(
       asr::EnvironmentFactory::create(
            "sky",
           asr::ParamArray()
                .insert("environment_edf", "sky_edf")
                .insert("environment_shader", "sky_shader")));

    //------------------------------------------------------------------------
    // Camera
    //------------------------------------------------------------------------

    // Create a pinhole camera with film dimensions 0.980 x 0.735 in (24.892 x 18.669 mm).
    asf::auto_release_ptr<asr::Camera> camera(
        asr::PinholeCameraFactory().create(
            "camera",
            asr::ParamArray()
                .insert("film_dimensions", "0.024892 0.018669")
                .insert("focal_length", "3.5")));

    // Place and orient the camera. By default cameras are located in (0.0, 0.0, 0.0)
    // and are looking toward Z- (0.0, 0.0, -1.0).
     asf::Matrix4d cam_tf = asf::Matrix4d::make_identity();
    // cam_tf(0,0) = 1;
    // cam_tf(1,1) = -1;
    // cam_tf(2,2) = -1;

    // cam_tf(1,3) = 0;
    
     cam_tf(2,3) = 0;

    camera->transform_sequence().set_transform(
        0.0,
        asf::Transformd::from_local_to_parent(cam_tf));

    // Bind the camera to the scene.
    scene->cameras().insert(camera);

    //------------------------------------------------------------------------
    // Frame
    //------------------------------------------------------------------------

    // Create a frame and bind it to the project.
    project->set_frame(
        asr::FrameFactory::create(
            "beauty",
            asr::ParamArray()
                .insert("camera", "camera")
                .insert("resolution", "640 480")
                .insert("color_space", "srgb")));

    // Bind the scene to the project.
    project->set_scene(scene);

    return project;
}
int main(int argc, char **argv)
{
    // Create a log target that outputs to stderr, and binds it to the renderer's global logger.
    // Eventually you will probably want to redirect log messages to your own target. For this
    // you will need to implement foundation::ILogTarget (foundation/utility/log/ilogtarget.h).
    int length, i = 0;
    int fd;
    int wd;
    char buffer[EVENT_BUF_LEN];



  


    fd = inotify_init();
   wd = inotify_add_watch(fd, "/tmp/test_ipc",
        IN_MODIFY | IN_CREATE | IN_DELETE);

    std::unique_ptr<asf::ILogTarget> log_target(asf::create_console_log_target(stderr));
    asr::global_logger().add_target(log_target.get());

    // Print appleseed's version string.
    RENDERER_LOG_INFO("%s", asf::Appleseed::get_synthetic_version_string());
float sat_pose[4] = {0};
float sat_pos[3] = {0};
while(true){
   wd = inotify_add_watch(fd, "/tmp/test_ipc",
        IN_MODIFY | IN_CREATE | IN_DELETE);
	std::cout << "wait for file" <<std::endl;
 length = read(fd, buffer, EVENT_BUF_LEN);
	std::cout << "file changed" << std::endl;


	std::ifstream file(  "/tmp/test_ipc" );
        std::stringstream ssbuffer;
	ssbuffer << file.rdbuf();
        file.close();

	if(ssbuffer.str().size() < 20)
	{
        std::cout << "FILE"<< ssbuffer.str()<<"ENDFILE" << std::endl;
	continue;
		}
  
       boost::property_tree::ptree pt;
        boost::property_tree::read_json(ssbuffer, pt);


	i=0;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("sat_pose"))
        {
            assert(v.first.empty()); // array elements have no names
            sat_pose[i++] = v.second.get_value<float>();
        }

	i=0;
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("sat_position"))
        {
            assert(v.first.empty()); // array elements have no names
            sat_pos[i++] = v.second.get_value<float>();
        }
	std::cout << sat_pose[0] << ","<< sat_pose[1] << ","<< sat_pose[2] << ","<< sat_pose[3] << "," << std::endl;
   
  // Build the project.
    asf::auto_release_ptr<asr::Project> project(build_project(sat_pose,sat_pos));

  /* std::cout << length << "bla" <<std::endl; 


    asr::ObjectInstance* obj =    project->get_scene()->assemblies().get_by_name("assembly")->object_instances().get_by_name("teeest");
   const std::string object_name(obj->get_object_name());
     std::cout << "NUM THINGS=" <<   project->get_scene()->assemblies().get_by_name("assembly")->object_instances().size() << std::endl;
     project->get_scene()->assemblies().get_by_name("assembly")->object_instances().remove(obj);
    obj = NULL;

     std::cout << "NUM THINGS=" <<   project->get_scene()->assemblies().get_by_name("assembly")->object_instances().size() << std::endl;
	std::cout << object_name << std::endl;

asf::Quaterniond quat_pose;
    quat_pose.s = sat_pose[3];
    quat_pose.v[0] = sat_pose[0];
    quat_pose.v[1] = sat_pose[1];
    quat_pose.v[2] = sat_pose[2];



  const asf::Transformd ExpectedTransform(	            
asf::Transformd::from_local_to_parent(asf::Matrix4d::make_rotation(asf::Vector3d(1.0,0.0, 0.0), sat_pose[3])));

 const std::string instance_name = "teeest";
         asf::auto_release_ptr<asr::ObjectInstance> obj2 = asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                asr::ParamArray(),
                object_name.c_str(),
                ExpectedTransform,
                asf::StringDictionary()
                    .insert("default", "light_material")
                    .insert("default2", "light_material"));

        project->get_scene()->assemblies().get_by_name("assembly")->object_instances().insert(obj2);

	std::cout << object_name << std::endl;

     std::cout << "NUM THINGS=" <<   project->get_scene()->assemblies().get_by_name("assembly")->object_instances().size() << std::endl;

   
*/
    
    // Create the master renderer.
    asr::DefaultRendererController renderer_controller;
    std::unique_ptr<asr::MasterRenderer> renderer(
        new asr::MasterRenderer(
            project.ref(),
            project->configurations().get_by_name("final")->get_inherited_parameters(),
            &renderer_controller));
  

    // Render the frame.
    renderer->render();
    project->get_frame()->write_main_image("/tmp/test.png");
//asr::ProjectFileWriter::write(project.ref(), "/tmp/test.appleseed");
    renderer.reset();
 	}
    
    // Save the project to disk.
   // asr::ProjectFileWriter::write(project.ref(), "output/test.appleseed");

    // Make sure to delete the master renderer before the project and the logger / log target.


    return 0;
}
