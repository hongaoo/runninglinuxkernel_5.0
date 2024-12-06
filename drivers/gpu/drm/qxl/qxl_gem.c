/*
 * Copyright 2013 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alon Levy
 */

#include <drm/drmP.h>
#include <drm/drm.h>
#include "../lib/libbmp.h"
#include "../lib/libpgm.h"

#include "qxl_drv.h"
#include "qxl_object.h"

void qxl_gem_object_free(struct drm_gem_object *gobj)
{
	struct qxl_bo *qobj = gem_to_qxl_bo(gobj);
	struct qxl_device *qdev;
	struct ttm_buffer_object *tbo;

	qdev = (struct qxl_device *)gobj->dev->dev_private;

	qxl_surface_evict(qdev, qobj, false);

	tbo = &qobj->tbo;
	ttm_bo_put(tbo);
}


/**
 * drm_gem_cma_print_info() - Print &drm_gem_cma_object info for debugfs
 * @p: DRM printer
 * @indent: Tab indentation level
 * @obj: GEM object
 *
 * This function can be used as the &drm_driver->gem_print_info callback.
 * It prints paddr and vaddr for use in e.g. debugfs output.
 */
void qxl_gem_print_info(struct drm_printer *p, unsigned int indent,
			    const struct drm_gem_object *obj)
{
	struct qxl_bo *qobj = gem_to_qxl_bo(obj);
	void *user_ptr;
	int ret;
	char filename[50] = {0};

	
	ret = qxl_bo_kmap(qobj, &user_ptr);

	drm_printf_indent(p, indent, "kptr=%lx\n", qobj->kptr);

	snprintf(filename, sizeof(filename), "/tmp/output_0x%lx.pgm",user_ptr);
	
	DRM_INFO("save pgm to file %s\n",filename);
	//save_bmp("/tmp/output.bmp", user_ptr, 100, 100);
	save_pgm_to_file(filename, user_ptr, 1024, 768);

	qxl_bo_kunmap(qobj);
}

void qxl_gem_free_object(struct drm_gem_object *gem_obj)
{
	printk("not implement");
}

static const struct drm_gem_object_funcs qxl_gem_default_funcs = {
	.free = qxl_gem_free_object,
	.print_info = qxl_gem_print_info,
 	.get_sg_table = NULL,
 	.vmap = NULL,
 	.vm_ops = NULL,
 };

int qxl_gem_object_create(struct qxl_device *qdev, int size,
			  int alignment, int initial_domain,
			  bool discardable, bool kernel,
			  struct qxl_surface *surf,
			  struct drm_gem_object **obj)
{
	struct qxl_bo *qbo;
	int r;

	*obj = NULL;
	/* At least align on page size */
	if (alignment < PAGE_SIZE)
		alignment = PAGE_SIZE;
	r = qxl_bo_create(qdev, size, kernel, false, initial_domain, surf, &qbo);
	if (r) {
		if (r != -ERESTARTSYS)
			DRM_ERROR(
			"Failed to allocate GEM object (%d, %d, %u, %d)\n",
				  size, initial_domain, alignment, r);
		return r;
	}
	*obj = &qbo->gem_base;

	qbo->gem_base.funcs = &qxl_gem_default_funcs;

	mutex_lock(&qdev->gem.mutex);
	list_add_tail(&qbo->list, &qdev->gem.objects);
	mutex_unlock(&qdev->gem.mutex);

	return 0;
}

int qxl_gem_object_create_with_handle(struct qxl_device *qdev,
				      struct drm_file *file_priv,
				      u32 domain,
				      size_t size,
				      struct qxl_surface *surf,
				      struct qxl_bo **qobj,
				      uint32_t *handle)
{
	struct drm_gem_object *gobj;
	int r;

	BUG_ON(!qobj);
	BUG_ON(!handle);

	r = qxl_gem_object_create(qdev, size, 0,
				  domain,
				  false, false, surf,
				  &gobj);
	if (r)
		return -ENOMEM;
	r = drm_gem_handle_create(file_priv, gobj, handle);
	if (r)
		return r;
	/* drop reference from allocate - handle holds it now */
	*qobj = gem_to_qxl_bo(gobj);
	drm_gem_object_put_unlocked(gobj);
	return 0;
}

int qxl_gem_object_open(struct drm_gem_object *obj, struct drm_file *file_priv)
{
	return 0;
}

void qxl_gem_object_close(struct drm_gem_object *obj,
			  struct drm_file *file_priv)
{
}

void qxl_gem_init(struct qxl_device *qdev)
{
	INIT_LIST_HEAD(&qdev->gem.objects);
}

void qxl_gem_fini(struct qxl_device *qdev)
{
	qxl_bo_force_delete(qdev);
}
