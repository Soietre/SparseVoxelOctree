//
// Created by adamyuan on 19-5-4.
//

#include "Voxelizer.hpp"
#include "Config.hpp"
#include "OglBindings.hpp"

void Voxelizer::Initialize()
{
	m_counter.Initialize();
	m_shader.Initialize();
	m_shader.LoadFromFile("shaders/voxelizer.frag", GL_FRAGMENT_SHADER);
	m_shader.LoadFromFile("shaders/voxelizer.vert", GL_VERTEX_SHADER);
	m_shader.LoadFromFile("shaders/voxelizer.geom", GL_GEOMETRY_SHADER);
	m_unif_count_only = m_shader.GetUniform("uCountOnly");
	m_unif_voxel_resolution = m_shader.GetUniform("uVoxelResolution");
	m_shader.SetInt(m_unif_voxel_resolution, kVoxelResolution);

	//Generate a 8x MSAA render buffer for MSAA Voxelization
	m_rbo.Initialize();
	glNamedRenderbufferStorageMultisample(m_rbo.Get(), 8, GL_R8, kVoxelResolution, kVoxelResolution);

	m_fbo.Initialize();
	m_fbo.AttachRenderbuffer(m_rbo, GL_COLOR_ATTACHMENT0);
}

void Voxelizer::Voxelize(const Scene &scene)
{
	m_fbo.Bind();
	glViewport(0, 0, kVoxelResolution, kVoxelResolution);

	m_counter.BindAtomicCounter(kAtomicCounterBinding);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	m_shader.Use();

	//fragment list not initialized, first count the voxels
	if(!mygl3::IsValidOglId(m_fragment_list.Get()))
	{
		m_counter.Reset();
		m_shader.SetInt(m_unif_count_only, 1);
		scene.Draw();

		m_fragment_num = m_counter.SyncAndGetValue();
		m_fragment_list.Initialize();
		m_fragment_list.Storage(m_fragment_num * sizeof(GLuint) * 2, 0);
		printf("[VOXELIZER]Info: Created fragment buffer with %d voxels, %.1lf MB\n", m_fragment_num,
			   m_fragment_list.GetByteCount() / 1000000.0);
	}

	m_counter.Reset();
	m_fragment_list.BindBase(GL_SHADER_STORAGE_BUFFER, kVoxelFragmentListSSBO);
	m_shader.SetInt(m_unif_count_only, 0);
	scene.Draw();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

	printf("[VOXELIZER]Info: Fragment buffer filled\n");

	mygl3::FrameBuffer::Unbind();
	glViewport(0, 0, kWidth, kHeight);
}

