/* DMLITE private methods for handle */

/* DMLITE private methods for export */

int dmlite_get_root_fd(struct fsal_export *exp_hdl);

struct dm_manager * dmlite_get_manager(struct fsal_export *exp_hdl);

/* Method proto linkage to handle.c for export */

fsal_status_t dmlite_lookup_path(struct fsal_export *exp_hdl,
			      const char *path,
			      struct fsal_obj_handle **handle);

fsal_status_t dmlite_create_handle(struct fsal_export *exp_hdl,
				struct gsh_buffdesc *hdl_desc,
				struct fsal_obj_handle **handle);


struct dmlite_fsal_obj_handle {
	struct fsal_obj_handle obj_handle;
	struct file_handle *handle;
	union {
		struct {
			int fd;
			fsal_openflags_t openflags;
			uint32_t lock_status; /* != 0, locks in play */
		} file;
		struct {
			unsigned char *link_content;
			int link_size;
		} symlink;
		struct {
			struct file_handle *sock_dir;
			char *sock_name;
		} sock;
	} u;
};


/* I/O management */
fsal_status_t dmlite_open(struct fsal_obj_handle *obj_hdl,
		       fsal_openflags_t openflags);
fsal_openflags_t dmlite_status(struct fsal_obj_handle *obj_hdl);
fsal_status_t dmlite_read(struct fsal_obj_handle *obj_hdl,
		       uint64_t offset,
		       size_t buffer_size,
		       void *buffer,
		       size_t *read_amount,
		       bool_t *end_of_file);
fsal_status_t dmlite_write(struct fsal_obj_handle *obj_hdl,
                        uint64_t offset,
			size_t buffer_size,
			void *buffer,
			size_t * write_amount);
fsal_status_t dmlite_commit(struct fsal_obj_handle *obj_hdl, /* sync */
			 off_t offset,
			 size_t len);
fsal_status_t dmlite_lock_op(struct fsal_obj_handle *obj_hdl,
			  void * p_owner,
			  fsal_lock_op_t lock_op,
			  fsal_lock_param_t   request_lock,
			  fsal_lock_param_t * conflicting_lock);
fsal_status_t dmlite_share_op(struct fsal_obj_handle *obj_hdl,
			   void *p_owner,         /* IN (opaque to FSAL) */
			   fsal_share_param_t  request_share);
fsal_status_t dmlite_close(struct fsal_obj_handle *obj_hdl);
fsal_status_t dmlite_lru_cleanup(struct fsal_obj_handle *obj_hdl,
			      lru_actions_t requests);

/* extended attributes management */
fsal_status_t dmlite_list_ext_attrs(struct fsal_obj_handle *obj_hdl,
				 unsigned int cookie,
				 fsal_xattrent_t * xattrs_tab,
				 unsigned int xattrs_tabsize,
				 unsigned int *p_nb_returned,
				 int *end_of_list);
fsal_status_t dmlite_getextattr_id_by_name(struct fsal_obj_handle *obj_hdl,
					const char *xattr_name,
					unsigned int *pxattr_id);
fsal_status_t dmlite_getextattr_value_by_name(struct fsal_obj_handle *obj_hdl,
					   const char *xattr_name,
					   caddr_t buffer_addr,
					   size_t buffer_size,
					   size_t * p_output_size);
fsal_status_t dmlite_getextattr_value_by_id(struct fsal_obj_handle *obj_hdl,
					 unsigned int xattr_id,
					 caddr_t buffer_addr,
					 size_t buffer_size,
					 size_t *p_output_size);
fsal_status_t dmlite_setextattr_value(struct fsal_obj_handle *obj_hdl,
				   const char *xattr_name,
				   caddr_t buffer_addr,
				   size_t buffer_size,
				   int create);
fsal_status_t dmlite_setextattr_value_by_id(struct fsal_obj_handle *obj_hdl,
					 unsigned int xattr_id,
					 caddr_t buffer_addr,
					 size_t buffer_size);
fsal_status_t dmlite_getextattr_attrs(struct fsal_obj_handle *obj_hdl,
				   unsigned int xattr_id,
				   struct attrlist *p_attrs);
fsal_status_t dmlite_remove_extattr_by_id(struct fsal_obj_handle *obj_hdl,
				       unsigned int xattr_id);
fsal_status_t dmlite_remove_extattr_by_name(struct fsal_obj_handle *obj_hdl,
					 const char *xattr_name);
