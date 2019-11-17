/*
 * Master File Table (MFT) entry functions
 *
 * Copyright (C) 2010-2019, Joachim Metz <joachim.metz@gmail.com>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <byte_stream.h>
#include <memory.h>
#include <types.h>

#include "libfsntfs_attribute_list.h"
#include "libfsntfs_attribute_list_entry.h"
#include "libfsntfs_bitmap_values.h"
#include "libfsntfs_debug.h"
#include "libfsntfs_definitions.h"
#include "libfsntfs_directory_entry.h"
#include "libfsntfs_file_name_values.h"
#include "libfsntfs_fixup_values.h"
#include "libfsntfs_io_handle.h"
#include "libfsntfs_libbfio.h"
#include "libfsntfs_libcdata.h"
#include "libfsntfs_libcerror.h"
#include "libfsntfs_libcnotify.h"
#include "libfsntfs_libfcache.h"
#include "libfsntfs_libfdata.h"
#include "libfsntfs_libuna.h"
#include "libfsntfs_mft_attribute.h"
#include "libfsntfs_mft_entry.h"
#include "libfsntfs_mft_entry_header.h"
#include "libfsntfs_types.h"
#include "libfsntfs_unused.h"

#include "fsntfs_mft_entry.h"

const char fsntfs_mft_entry_signature[ 4 ] = "FILE";

/* Checks if a buffer containing the MFT entry is filled with 0-byte values (empty-block)
 * Returns 1 if empty, 0 if not or -1 on error
 */
int libfsntfs_mft_entry_check_for_empty_block(
     const uint8_t *data,
     size_t data_size,
     libcerror_error_t **error )
{
	libfsntfs_aligned_t *aligned_data_index = NULL;
	libfsntfs_aligned_t *aligned_data_start = NULL;
	uint8_t *data_index                     = NULL;
	uint8_t *data_start                     = NULL;
	static char *function                   = "libfsntfs_mft_entry_check_for_empty_block";

	if( data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid data.",
		 function );

		return( -1 );
	}
	if( data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	data_start = (uint8_t *) data;
	data_index = (uint8_t *) data + 1;
	data_size -= 1;

	/* Only optimize for data larger than the alignment
	 */
	if( data_size > ( 2 * sizeof( libfsntfs_aligned_t ) ) )
	{
		/* Align the data start
		 */
		while( ( (intptr_t) data_start % sizeof( libfsntfs_aligned_t ) ) != 0 )
		{
			if( *data_start != *data_index )
			{
				return( 0 );
			}
			data_start += 1;
			data_index += 1;
			data_size  -= 1;
		}
		/* Align the data index
		 */
		while( ( (intptr_t) data_index % sizeof( libfsntfs_aligned_t ) ) != 0 )
		{
			if( *data_start != *data_index )
			{
				return( 0 );
			}
			data_index += 1;
			data_size  -= 1;
		}
		aligned_data_start = (libfsntfs_aligned_t *) data_start;
		aligned_data_index = (libfsntfs_aligned_t *) data_index;

		while( data_size > sizeof( libfsntfs_aligned_t ) )
		{
			if( *aligned_data_start != *aligned_data_index )
			{
				return( 0 );
			}
			aligned_data_index += 1;
			data_size          -= sizeof( libfsntfs_aligned_t );
		}
		data_index = (uint8_t *) aligned_data_index;
	}
	while( data_size != 0 )
	{
		if( *data_start != *data_index )
		{
			return( 0 );
		}
		data_index += 1;
		data_size  -= 1;
	}
	return( 1 );
}

/* Creates a MFT entry
 * Make sure the value mft_entry is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_initialize(
     libfsntfs_mft_entry_t **mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_initialize";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( *mft_entry != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid MFT entry value already set.",
		 function );

		return( -1 );
	}
	*mft_entry = memory_allocate_structure(
	              libfsntfs_mft_entry_t );

	if( *mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create MFT entry.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *mft_entry,
	     0,
	     sizeof( libfsntfs_mft_entry_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear MFT entry.",
		 function );

		memory_free(
		 *mft_entry );

		*mft_entry = NULL;

		return( -1 );
	}
	if( libcdata_array_initialize(
	     &( ( *mft_entry )->attributes_array ),
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create attributes array.",
		 function );

		goto on_error;
	}
	if( libcdata_array_initialize(
	     &( ( *mft_entry )->alternate_data_attributes_array ),
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create alternate data attributes array.",
		 function );

		goto on_error;
	}
	( *mft_entry )->file_name_attribute_index            = -1;
	( *mft_entry )->reparse_point_attribute_index        = -1;
	( *mft_entry )->security_descriptor_attribute_index  = -1;
	( *mft_entry )->standard_information_attribute_index = -1;
	( *mft_entry )->volume_information_attribute_index   = -1;
	( *mft_entry )->volume_name_attribute_index          = -1;

	return( 1 );

on_error:
	if( *mft_entry != NULL )
	{
		if( ( *mft_entry )->alternate_data_attributes_array != NULL )
		{
			libcdata_array_free(
			 &( ( *mft_entry )->alternate_data_attributes_array ),
			 NULL,
			 NULL );
		}
		if( ( *mft_entry )->attributes_array != NULL )
		{
			libcdata_array_free(
			 &( ( *mft_entry )->attributes_array ),
			 NULL,
			 NULL );
		}
		memory_free(
		 *mft_entry );

		*mft_entry = NULL;
	}
	return( -1 );
}

/* Frees a MFT entry
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_free(
     libfsntfs_mft_entry_t **mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_free";
	int result            = 1;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( *mft_entry != NULL )
	{
		if( ( *mft_entry )->header != NULL )
		{
			if( libfsntfs_mft_entry_header_free(
			     &( ( *mft_entry )->header ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free MFT entry header.",
				 function );

				result = -1;
			}
		}
		if( ( *mft_entry )->data != NULL )
		{
			memory_free(
			 ( *mft_entry )->data );
		}
		/* The specific attribute references point to attributes in the array
		 * and are freed by freeing the array and its values
		 */
		if( libcdata_array_free(
		     &( ( *mft_entry )->attributes_array ),
		     (int (*)(intptr_t **, libcerror_error_t **)) &libfsntfs_mft_attribute_free,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free attributes array.",
			 function );

			result = -1;
		}
		/* The alternate_data_attributes_array only contains references that are managed
		 * by the attributes_array
		 */
		if( libcdata_array_free(
		     &( ( *mft_entry )->alternate_data_attributes_array ),
		     NULL,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free alternate data attributes array.",
			 function );

			result = -1;
		}
		memory_free(
		 *mft_entry );

		*mft_entry = NULL;
	}
	return( result );
}

/* Reads the MFT entry
 * Returns 1 if successful, 0 if empty or marked as bad, or -1 on error
 */
int libfsntfs_mft_entry_read_data(
     libfsntfs_mft_entry_t *mft_entry,
     uint8_t *data,
     size_t data_size,
     uint32_t mft_entry_index,
     libcerror_error_t **error )
{
	static char *function           = "libfsntfs_mft_entry_read_data";
	size_t data_offset              = 0;
	size_t unknown_data_size        = 0;
	uint16_t attributes_offset      = 0;
	uint16_t fixup_values_offset    = 0;
	uint16_t number_of_fixup_values = 0;
	int result                      = 0;

#if defined( HAVE_DEBUG_OUTPUT )
	uint16_t total_entry_size       = 0;
#endif

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( mft_entry->header != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid MFT entry - header value already set.",
		 function );

		return( -1 );
	}
	if( data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid data.",
		 function );

		return( -1 );
	}
	if( data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	result = libfsntfs_mft_entry_check_for_empty_block(
	          data,
	          data_size,
	          error );

	if( result == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to determine if MFT entry is empty.",
		 function );

		goto on_error;
	}
	else if( result != 0 )
	{
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: MFT entry: %" PRIu32 " is empty.\n",
			 function,
			 mft_entry_index );
		}
#endif
		mft_entry->is_empty = 1;

		return( 0 );
	}
	if( libfsntfs_mft_entry_header_initialize(
	     &( mft_entry->header ),
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create MFT entry header.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_entry_header_read_data(
	     mft_entry->header,
	     data,
	     data_size,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read MFT entry header.",
		 function );

		goto on_error;
	}
	if( mft_entry->header->is_bad != 0 )
	{
		mft_entry->is_empty = 1;

/* TODO do empty block check on the remainder of the MFT entry? */
		return( 0 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		if( libfsntfs_mft_entry_header_get_total_entry_size(
		     mft_entry->header,
		     &total_entry_size,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve total entry size.",
			 function );

			goto on_error;
		}
		if( data_size != (size_t) total_entry_size )
		{
			libcnotify_printf(
			 "%s: mismatch in total MFT entry size (calculated: %" PRIzd ", stored: %" PRIu16 ").\n",
			 function,
			 data_size,
			 total_entry_size );
		}
	}
#endif
	if( libfsntfs_mft_entry_header_get_attributes_offset(
	     mft_entry->header,
	     &attributes_offset,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve attributes offset.",
		 function );

		goto on_error;
	}
	if( attributes_offset >= data_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid attributes offset value out of bounds.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_entry_header_get_fixup_values_offset(
	     mft_entry->header,
	     &fixup_values_offset,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve fix-up values offset.",
		 function );

		goto on_error;
	}
	if( fixup_values_offset > attributes_offset )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: fix-up values offset exceeds attributes offset.",
		 function );

		goto on_error;
	}
	if( fixup_values_offset > 42 )
	{
		data_offset = sizeof( fsntfs_mft_entry_header_t );
	}
	else
	{
		/* In NTFS 1.2 the fix-up values offset can point to wfixupPattern
		 */
		data_offset = 42;
	}
	if( data_offset < fixup_values_offset )
	{
		unknown_data_size = (size_t) fixup_values_offset - data_offset;

#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: unknown data:\n",
			 function );
			libcnotify_print_data(
			 &( data[ data_offset ] ),
			 unknown_data_size,
			 0 );
		}
#endif
		data_offset += unknown_data_size;
	}
	if( libfsntfs_mft_entry_header_get_number_of_fixup_values(
	     mft_entry->header,
	     &number_of_fixup_values,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of fix-up values.",
		 function );

		goto on_error;
	}
	if( number_of_fixup_values > 0 )
	{
		if( libfsntfs_fixup_values_apply(
		     data,
		     data_size,
		     fixup_values_offset,
		     number_of_fixup_values,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to apply fix-up values.",
			 function );

			goto on_error;
		}
		data_offset += 2 + ( (size_t) number_of_fixup_values * 2 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		if( data_offset < attributes_offset )
		{
			libcnotify_printf(
			 "%s: unknown data:\n",
			 function );
			libcnotify_print_data(
			 &( data[ data_offset ] ),
			 (size_t) attributes_offset - data_offset,
			 0 );
		}
	}
#endif
	mft_entry->is_empty = 0;

	mft_entry->index = mft_entry->header->index;

	if( mft_entry->index != mft_entry_index )
	{
		mft_entry->index = mft_entry_index;
	}
	mft_entry->file_reference = ( (uint64_t) mft_entry->header->sequence << 48 ) | mft_entry->index;

	return( 1 );

on_error:
	if( mft_entry->header != NULL )
	{
		libfsntfs_mft_entry_header_free(
		 &( mft_entry->header ),
		 NULL );
	}
	return( -1 );
}

/* Reads the MFT entry
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_read_file_io_handle(
     libfsntfs_mft_entry_t *mft_entry,
     libbfio_handle_t *file_io_handle,
     off64_t file_offset,
     uint32_t mft_entry_size,
     uint32_t mft_entry_index,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_read_file_io_handle";
	ssize_t read_count    = 0;
	int result            = 0;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( mft_entry->data != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid MFT entry - data value already set.",
		 function );

		return( -1 );
	}
#if ( SIZEOF_SIZE_T <= 4 )
	if( ( (size_t) mft_entry_size <= 42 )
	 || ( (size_t) mft_entry_size > (size_t) SSIZE_MAX ) )
#else
	if( (size_t) mft_entry_size <= 42 )
#endif
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid MFT entry size value out of bounds.",
		 function );

		goto on_error;
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: reading MFT entry at offset: %" PRIi64 " (0x%08" PRIx64 ")\n",
		 function,
		 file_offset,
		 file_offset );
	}
#endif
	if( libbfio_handle_seek_offset(
	     file_io_handle,
	     file_offset,
	     SEEK_SET,
	     error ) == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek MFT entry offset: %" PRIi64 ".",
		 function,
		 file_offset );

		goto on_error;
	}
	mft_entry->data = (uint8_t *) memory_allocate(
	                               (size_t) mft_entry_size );

	if( mft_entry->data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create MFT entry data.",
		 function );

		goto on_error;
	}
	mft_entry->data_size = (size_t) mft_entry_size;

	read_count = libbfio_handle_read_buffer(
	              file_io_handle,
	              mft_entry->data,
	              mft_entry->data_size,
	              error );

	if( read_count != (ssize_t) mft_entry->data_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read MFT entry data.",
		 function );

		goto on_error;
	}
	result = libfsntfs_mft_entry_read_data(
	          mft_entry,
	          mft_entry->data,
	          mft_entry->data_size,
	          mft_entry_index,
	          error );

	if( result == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read MFT entry data.",
		 function );

		goto on_error;
	}
	else if( result == 0 )
	{
		memory_free(
		 mft_entry->data );

		mft_entry->data      = NULL;
		mft_entry->data_size = 0;
	}
	return( 1 );

on_error:
	if( mft_entry->header != NULL )
	{
		libfsntfs_mft_entry_header_free(
		 &( mft_entry->header ),
		 NULL );
	}
	if( mft_entry->data != NULL )
	{
		memory_free(
		 mft_entry->data );

		mft_entry->data = NULL;
	}
	mft_entry->data_size = 0;

	return( -1 );
}

/* Reads the MFT attributes
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_read_attributes_data(
     libfsntfs_mft_entry_t *mft_entry,
     libbfio_handle_t *file_io_handle,
     libfsntfs_io_handle_t *io_handle,
     const uint8_t *data,
     size_t data_size,
     uint8_t flags,
     libcerror_error_t **error )
{
	libfsntfs_mft_attribute_t *mft_attribute = NULL;
	static char *function                    = "libfsntfs_mft_entry_read_attributes_data";
	size_t data_offset                       = 0;
	uint32_t attribute_type                  = 0;
	uint16_t attributes_offset               = 0;
	int attribute_index                      = 0;

#if defined( HAVE_DEBUG_OUTPUT )
	uint16_t used_entry_size                 = 0;
#endif

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid data.",
		 function );

		return( -1 );
	}
	if( data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( data_size < 4 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
		 "%s: unsupported data size value too small\n",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_entry_header_get_attributes_offset(
	     mft_entry->header,
	     &attributes_offset,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve attributes offset.",
		 function );

		goto on_error;
	}
	data_offset = (size_t) attributes_offset;

	do
	{
		if( data_offset > ( data_size - 4 ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid MFT entry - attribute offset: %d value out of bounds.",
			 function,
			 attribute_index );

			goto on_error;
		}
		byte_stream_copy_to_uint32_little_endian(
		 &( data[ data_offset ] ),
		 attribute_type );

		if( attribute_type == LIBFSNTFS_ATTRIBUTE_TYPE_END_OF_ATTRIBUTES )
		{
			break;
		}
		if( libfsntfs_mft_attribute_initialize(
		     &mft_attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create MFT attribute: %d.",
			 function,
			 attribute_index );

			goto on_error;
		}
		if( libfsntfs_mft_attribute_read_data(
		     mft_attribute,
		     io_handle,
		     &( data[ data_offset ] ),
		     data_size - data_offset,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read MFT attribute: %d of type: 0x08" PRIx32 ".",
			 function,
			 attribute_index,
			 attribute_type );

			goto on_error;
		}
		data_offset += mft_attribute->size;

		if( attribute_type == LIBFSNTFS_ATTRIBUTE_TYPE_ATTRIBUTE_LIST )
		{
			if( mft_entry->list_attribute != NULL )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
				 "%s: invalid MFT entry - list attribute value already set.",
				 function );

				goto on_error;
			}
			mft_entry->list_attribute = mft_attribute;
		}
		if( libfsntfs_mft_entry_append_attribute(
		     mft_entry,
		     io_handle,
		     file_io_handle,
		     mft_attribute,
		     flags,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
			 "%s: unable to append attribute: %d.",
			 function,
			 attribute_index );

			goto on_error;
		}
		mft_attribute = NULL;

		attribute_index++;
	}
	while( attribute_type != LIBFSNTFS_ATTRIBUTE_TYPE_END_OF_ATTRIBUTES );

	data_offset += 4;

#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		if( libfsntfs_mft_entry_header_get_used_entry_size(
		     mft_entry->header,
		     &used_entry_size,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve used entry size.",
			 function );

			goto on_error;
		}
		if( data_offset != (size_t) used_entry_size )
		{
			libcnotify_printf(
			 "%s: mismatch in used MFT entry size (calculated: %" PRIzd ", stored: %" PRIu16 ").\n",
			 function,
			 data_offset,
			 used_entry_size );
		}
	}
#endif
	return( 1 );

on_error:
	mft_entry->list_attribute = NULL;

	if( mft_attribute != NULL )
	{
		libfsntfs_mft_attribute_free(
		 &mft_attribute,
		 NULL );
	}
	libcdata_array_empty(
	 mft_entry->attributes_array,
	 (int (*)(intptr_t **, libcerror_error_t **)) &libfsntfs_mft_attribute_free,
	 NULL );

	return( -1 );
}

/* Reads the MFT attributes
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_read_attributes(
     libfsntfs_mft_entry_t *mft_entry,
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfdata_vector_t *mft_entry_vector,
     uint8_t flags,
     libcerror_error_t **error )
{
	libcdata_array_t *attribute_list = NULL;
	static char *function            = "libfsntfs_mft_entry_read_attributes";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
/* TODO refactor need to pass file_io_handle, io_handle and flags */
	if( libfsntfs_mft_entry_read_attributes_data(
	     mft_entry,
	     file_io_handle,
	     io_handle,
	     mft_entry->data,
	     mft_entry->data_size,
	     flags,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read MFT attributes.",
		 function );

		goto on_error;
	}
/* TODO if flag is set mark MFT entry as incomplete ? */
	if( ( mft_entry->list_attribute != NULL )
	 && ( ( flags & LIBFSNTFS_FILE_ENTRY_FLAGS_MFT_ONLY ) == 0 ) )
	{
		if( libcdata_array_initialize(
		     &attribute_list,
		     0,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create attribute list array.",
			 function );

			goto on_error;
		}
		if( libfsntfs_attribute_list_read_from_attribute(
		     attribute_list,
		     io_handle,
		     file_io_handle,
		     mft_entry->list_attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read attribute list.",
			 function );

			goto on_error;
		}
		if( libfsntfs_mft_entry_read_attributes_from_attribute_list(
		     mft_entry,
		     io_handle,
		     file_io_handle,
		     mft_entry_vector,
		     attribute_list,
		     flags,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read attributes from attribute list.",
			 function );

			goto on_error;
		}
		if( libcdata_array_free(
		     &attribute_list,
		     (int (*)(intptr_t **, libcerror_error_t **)) &libfsntfs_attribute_list_entry_free,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free attribute list array.",
			 function );

			goto on_error;
		}
	}
	return( 1 );

on_error:
	if( attribute_list != NULL )
	{
		libcdata_array_free(
		 &attribute_list,
		 (int (*)(intptr_t **, libcerror_error_t **)) &libfsntfs_attribute_list_entry_free,
		 NULL );
	}
	libcdata_array_empty(
	 mft_entry->alternate_data_attributes_array,
	 NULL,
	 NULL );

	libcdata_array_empty(
	 mft_entry->attributes_array,
	 (int (*)(intptr_t **, libcerror_error_t **)) &libfsntfs_mft_attribute_free,
	 NULL );

	return( -1 );
}

/* Reads attributes from an attribute list
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_read_attributes_from_attribute_list(
     libfsntfs_mft_entry_t *mft_entry,
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfdata_vector_t *mft_entry_vector,
     libcdata_array_t *attribute_list,
     uint8_t flags,
     libcerror_error_t **error )
{
	libcdata_array_t *data_mft_entry_array                 = NULL;
	libfcache_cache_t *mft_entry_cache                     = NULL;
	libfsntfs_attribute_list_entry_t *attribute_list_entry = NULL;
	libfsntfs_mft_attribute_t *mft_attribute               = NULL;
	libfsntfs_mft_entry_t *data_mft_entry                  = NULL;
	static char *function                                  = "libfsntfs_mft_entry_read_attributes_from_attribute_list";
	uint64_t data_mft_entry_index                          = 0;
	uint16_t sequence_number                               = 0;
	int attribute_index                                    = 0;
	int attribute_list_entry_index                         = 0;
	int entry_index                                        = 0;
	int number_of_attribute_list_entries                   = 0;
	int number_of_attributes                               = 0;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	/* Determine the individual data MFT entries in the attribute list
	 */
	if( libcdata_array_initialize(
	     &data_mft_entry_array,
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create data MFT entry array.",
		 function );

		goto on_error;
	}
	if( libcdata_array_get_number_of_entries(
	     attribute_list,
	     &number_of_attribute_list_entries,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of attribute list entries.",
		 function );

		goto on_error;
	}
	for( attribute_list_entry_index = 0;
	     attribute_list_entry_index < number_of_attribute_list_entries;
	     attribute_list_entry_index++ )
	{
		if( libcdata_array_get_entry_by_index(
		     attribute_list,
		     attribute_list_entry_index,
		     (intptr_t **) &attribute_list_entry,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve attribute list entry: %d.",
			 function,
			 attribute_list_entry_index );

			goto on_error;
		}
		if( libfsntfs_attribute_list_entry_get_file_reference(
		     attribute_list_entry,
		     &data_mft_entry_index,
		     &sequence_number,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve attribute list entry: %d file reference.",
			 function,
			 attribute_list_entry_index );

			goto on_error;
		}
		if( data_mft_entry_index > (uint64_t) INT_MAX )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: data MFT entry reference value out of bounds.",
			 function );

			goto on_error;
		}
		/* Ignore the current MFT entry
		 */
		if( data_mft_entry_index == (uint64_t) mft_entry->index )
		{
			continue;
		}
		if( libcdata_array_insert_entry(
		     data_mft_entry_array,
		     &entry_index,
		     (intptr_t *) attribute_list_entry,
		     (int (*)(intptr_t *, intptr_t *, libcerror_error_t **)) &libfsntfs_attribute_list_entry_compare_by_file_reference,
		     LIBCDATA_INSERT_FLAG_UNIQUE_ENTRIES,
		     error ) == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
			 "%s: unable to insert attribute list entry in data MFT entry array.",
			 function );

			goto on_error;
		}
	}
	/* Read the data MFT entries
	 * Use a local cache to prevent cache outs
	 */
	if( libfcache_cache_initialize(
	     &mft_entry_cache,
	     1,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create MFT entry cache.",
		 function );

		goto on_error;
	}
	if( libcdata_array_get_number_of_entries(
	     data_mft_entry_array,
	     &number_of_attribute_list_entries,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of attribute list entries.",
		 function );

		goto on_error;
	}
	for( attribute_list_entry_index = 0;
	     attribute_list_entry_index < number_of_attribute_list_entries;
	     attribute_list_entry_index++ )
	{
		if( libcdata_array_get_entry_by_index(
		     data_mft_entry_array,
		     attribute_list_entry_index,
		     (intptr_t **) &attribute_list_entry,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve attribute list entry: %d.",
			 function,
			 attribute_list_entry_index );

			goto on_error;
		}
		if( libfsntfs_attribute_list_entry_get_file_reference(
		     attribute_list_entry,
		     &data_mft_entry_index,
		     &sequence_number,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve attribute list entry: %d file reference.",
			 function,
			 attribute_list_entry_index );

			goto on_error;
		}
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: reading data file reference: MFT entry: %" PRIu64 ", sequence: %" PRIu16 "\n",
			 function,
			 data_mft_entry_index,
			 sequence_number );
			libcnotify_printf(
			 "\n" );
		}
#endif
		if( libfdata_vector_get_element_value_by_index(
		     mft_entry_vector,
		     (intptr_t *) file_io_handle,
		     (libfdata_cache_t *) mft_entry_cache,
		     (int) data_mft_entry_index,
		     (intptr_t **) &data_mft_entry,
		     0,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve MFT entry: %" PRIu64 ".",
			 function,
			 data_mft_entry_index );

			goto on_error;
		}
		if( libcdata_array_get_number_of_entries(
		     data_mft_entry->attributes_array,
		     &number_of_attributes,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve number of attributes.",
			 function );

			goto on_error;
		}
		for( attribute_index = 0;
		     attribute_index < number_of_attributes;
		     attribute_index++ )
		{
			if( libcdata_array_get_entry_by_index(
			     data_mft_entry->attributes_array,
			     attribute_index,
			     (intptr_t **) &mft_attribute,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve attribute: %d from list MFT entry.",
				 function,
				 attribute_index );

				goto on_error;
			}
			if( libcdata_array_set_entry_by_index(
			     data_mft_entry->attributes_array,
			     attribute_index,
			     NULL,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to set attribute: %d in list MFT entry.",
				 function,
				 attribute_index );

				goto on_error;
			}
			if( libfsntfs_mft_entry_append_attribute(
			     mft_entry,
			     io_handle,
			     file_io_handle,
			     mft_attribute,
			     flags,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
				 "%s: unable to append attribute: %d from list MFT entry.",
				 function,
				 attribute_index );

				goto on_error;
			}
		}
	}
	if( libfcache_cache_free(
	     &mft_entry_cache,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free MFT entry cache.",
		 function );

		goto on_error;
	}
	if( libcdata_array_free(
	     &data_mft_entry_array,
	     NULL,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free data MFT entry array.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( mft_entry_cache != NULL )
	{
		libfcache_cache_free(
		 &mft_entry_cache,
		 NULL );
	}
	if( data_mft_entry_array != NULL )
	{
		libcdata_array_free(
		 &data_mft_entry_array,
		 NULL,
		 NULL );
	}
	return( -1 );
}

/* Determines if the MFT entry is empty
 * Returns 1 if empty, 0 if not or -1 on error
 */
int libfsntfs_mft_entry_is_empty(
     libfsntfs_mft_entry_t *mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_is_empty";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	return( (int) mft_entry->is_empty );
}

/* Determines if the MFT entry is allocated (in use)
 * Returns 1 if allocated, 0 if not or -1 on error
 */
int libfsntfs_mft_entry_is_allocated(
     libfsntfs_mft_entry_t *mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_is_allocated";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( ( mft_entry->flags & LIBFSNTFS_MFT_ENTRY_FLAG_IN_USE ) != 0 )
	{
		return( 1 );
	}
	return( 0 );
}

/* Retrieves the file reference
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_file_reference(
     libfsntfs_mft_entry_t *mft_entry,
     uint64_t *file_reference,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_file_reference";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( file_reference == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file reference.",
		 function );

		return( -1 );
	}
	*file_reference = mft_entry->file_reference;

	return( 1 );
}

/* Retrieves the base record file reference
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_base_record_file_reference(
     libfsntfs_mft_entry_t *mft_entry,
     uint64_t *file_reference,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_base_record_file_reference";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( file_reference == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file reference.",
		 function );

		return( -1 );
	}
	*file_reference = mft_entry->base_record_file_reference;

	return( 1 );
}

/* Retrieves the journal sequence number
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_journal_sequence_number(
     libfsntfs_mft_entry_t *mft_entry,
     uint64_t *journal_sequence_number,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_journal_sequence_number";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( journal_sequence_number == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid journal sequence number.",
		 function );

		return( -1 );
	}
	*journal_sequence_number = mft_entry->journal_sequence_number;

	return( 1 );
}

/* Retrieves the number of attributes
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_number_of_attributes(
     libfsntfs_mft_entry_t *mft_entry,
     int *number_of_attributes,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_number_of_attributes";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_number_of_entries(
	     mft_entry->attributes_array,
	     number_of_attributes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of entries from attributes array.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves a specific attribute
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_attribute_by_index(
     libfsntfs_mft_entry_t *mft_entry,
     int attribute_index,
     libfsntfs_mft_attribute_t **attribute,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_attribute_by_index";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_entry_by_index(
	     mft_entry->attributes_array,
	     attribute_index,
	     (intptr_t **) attribute,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve entry: %d from attributes array.",
		 function,
		 attribute_index );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves the number of alternate data attributes
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_number_of_alternate_data_attributes(
     libfsntfs_mft_entry_t *mft_entry,
     int *number_of_attributes,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_number_of_alternate_data_attributes";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_number_of_entries(
	     mft_entry->alternate_data_attributes_array,
	     number_of_attributes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of entries from alternate data attributes array.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves a specific alternate data attribute
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_get_alternate_data_attribute_by_index(
     libfsntfs_mft_entry_t *mft_entry,
     int attribute_index,
     libfsntfs_mft_attribute_t **attribute,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_get_alternate_data_attribute_by_index";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_entry_by_index(
	     mft_entry->alternate_data_attributes_array,
	     attribute_index,
	     (intptr_t **) attribute,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve entry: %d from alternate data attributes array.",
		 function,
		 attribute_index );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves a specific alternate data attribute for an UTF-8 encoded name
 * Returns 1 if successful, 0 if no such directory entry or -1 on error
 */
int libfsntfs_mft_entry_get_alternate_data_attribute_by_utf8_name(
     libfsntfs_mft_entry_t *mft_entry,
     const uint8_t *utf8_string,
     size_t utf8_string_length,
     libfsntfs_mft_attribute_t **attribute,
     libcerror_error_t **error )
{
	static char *function    = "libfsntfs_mft_entry_get_alternate_data_attribute_by_utf8_name";
	int attribute_index      = 0;
	int number_of_attributes = 0;
	int result               = 0;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( attribute == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_number_of_entries(
	     mft_entry->alternate_data_attributes_array,
	     &number_of_attributes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of entries from alternate data attributes array.",
		 function );

		return( -1 );
	}
	for( attribute_index = 0;
	     attribute_index < number_of_attributes;
	     attribute_index++ )
	{
		if( libcdata_array_get_entry_by_index(
		     mft_entry->alternate_data_attributes_array,
		     attribute_index,
		     (intptr_t **) attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve entry: %d from alternative data attributes array.",
			 function,
			 attribute_index );

			return( -1 );
		}
		result = libfsntfs_mft_attribute_compare_name_with_utf8_string(
			  *attribute,
			  utf8_string,
			  utf8_string_length,
			  error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GENERIC,
			 "%s: unable to compare UTF-8 string with alternative data attribute name.",
			 function );

			goto on_error;
		}
		else if( result == 1 )
		{
			return( 1 );
		}
	}
	*attribute = NULL;

	return( 0 );

on_error:
	*attribute = NULL;

	return( -1 );
}

/* Retrieves a specific alternate data attribute for an UTF-16 encoded name
 * Returns 1 if successful, 0 if no such directory entry or -1 on error
 */
int libfsntfs_mft_entry_get_alternate_data_attribute_by_utf16_name(
     libfsntfs_mft_entry_t *mft_entry,
     const uint16_t *utf16_string,
     size_t utf16_string_length,
     libfsntfs_mft_attribute_t **attribute,
     libcerror_error_t **error )
{
	static char *function    = "libfsntfs_mft_entry_get_alternate_data_attribute_by_utf16_name";
	int attribute_index      = 0;
	int number_of_attributes = 0;
	int result               = 0;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( attribute == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_number_of_entries(
	     mft_entry->alternate_data_attributes_array,
	     &number_of_attributes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of entries from alternate data attributes array.",
		 function );

		return( -1 );
	}
	for( attribute_index = 0;
	     attribute_index < number_of_attributes;
	     attribute_index++ )
	{
		if( libcdata_array_get_entry_by_index(
		     mft_entry->alternate_data_attributes_array,
		     attribute_index,
		     (intptr_t **) attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve entry: %d from alternative data attributes array.",
			 function,
			 attribute_index );

			return( -1 );
		}
		result = libfsntfs_mft_attribute_compare_name_with_utf16_string(
			  *attribute,
			  utf16_string,
			  utf16_string_length,
			  error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GENERIC,
			 "%s: unable to compare UTF-16 string with alternative data attribute name.",
			 function );

			goto on_error;
		}
		else if( result == 1 )
		{
			return( 1 );
		}
	}
	*attribute = NULL;

	return( 0 );

on_error:
	*attribute = NULL;

	return( -1 );
}

/* Appends an attribute
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_append_attribute(
     libfsntfs_mft_entry_t *mft_entry,
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfsntfs_mft_attribute_t *attribute,
     uint8_t flags,
     libcerror_error_t **error )
{
	uint8_t utf8_attribute_name[ 64 ];

	static char *function                    = "libfsntfs_mft_entry_append_attribute";
	size_t utf8_attribute_name_size          = 0;
	uint32_t attribute_type                  = 0;
	int entry_index                          = 0;
	int result                               = 0;

#if defined( HAVE_DEBUG_OUTPUT )
	libfsntfs_bitmap_values_t *bitmap_values = NULL;
#endif

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( attribute == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute.",
		 function );

		return( -1 );
	}
	if( libcdata_array_append_entry(
	     mft_entry->attributes_array,
	     &entry_index,
	     (intptr_t *) attribute,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
		 "%s: unable to append attribute to array.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_attribute_get_utf8_name_size(
	     attribute,
	     &utf8_attribute_name_size,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve UTF-8 attribute name size.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_attribute_get_type(
	     attribute,
	     &attribute_type,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve attribute type.",
		 function );

		goto on_error;
	}
	switch( attribute_type )
	{
		case LIBFSNTFS_ATTRIBUTE_TYPE_BITMAP:
#if defined( HAVE_DEBUG_OUTPUT )
			if( libcnotify_verbose != 0 )
			{
				if( libfsntfs_bitmap_values_initialize(
				     &bitmap_values,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
					 "%s: unable to create bitmap values.",
					 function );

					goto on_error;
				}
				if( libfsntfs_bitmap_values_read_from_mft_attribute(
				     bitmap_values,
				     attribute,
				     io_handle,
				     file_io_handle,
				     flags,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_IO,
					 LIBCERROR_IO_ERROR_READ_FAILED,
					 "%s: unable to read bitmap values from attribute.",
					 function );

					goto on_error;
				}
				if( libfsntfs_bitmap_values_free(
				     &bitmap_values,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
					 "%s: unable to free bitmap values.",
					 function );

					goto on_error;
				}
			}
#endif /* defined( HAVE_DEBUG_OUTPUT ) */
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_DATA:
			if( libfsntfs_mft_entry_append_data_attribute(
			     mft_entry,
			     attribute,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
				 "%s: unable to append data attribute to MFT entry.",
				 function );

				goto on_error;
			}
			break;

/* TODO refactor
		case LIBFSNTFS_ATTRIBUTE_TYPE_FILE_NAME:
#if defined( HAVE_DEBUG_OUTPUT )
		case LIBFSNTFS_ATTRIBUTE_TYPE_LOGGED_UTILITY_STREAM:
#endif
		case LIBFSNTFS_ATTRIBUTE_TYPE_OBJECT_IDENTIFIER:
		case LIBFSNTFS_ATTRIBUTE_TYPE_REPARSE_POINT:
		case LIBFSNTFS_ATTRIBUTE_TYPE_SECURITY_DESCRIPTOR:
		case LIBFSNTFS_ATTRIBUTE_TYPE_STANDARD_INFORMATION:
		case LIBFSNTFS_ATTRIBUTE_TYPE_VOLUME_NAME:
		case LIBFSNTFS_ATTRIBUTE_TYPE_VOLUME_INFORMATION:
			if( libfsntfs_attribute_read_value(
			     attribute,
			     io_handle,
			     file_io_handle,
			     flags,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_READ_FAILED,
				 "%s: unable to read attribute value of type: 0x%08" PRIx32 ".",
				 function,
				 attribute_type );

				goto on_error;
			}
			break;
*/

		case LIBFSNTFS_ATTRIBUTE_TYPE_FILE_NAME:
			if( mft_entry->file_name_attribute_index == -1 )
			{
				mft_entry->file_name_attribute_index = entry_index;
			}
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_INDEX_ALLOCATION:
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_INDEX_ROOT:
			if( libfsntfs_mft_attribute_get_utf8_name(
			     attribute,
			     utf8_attribute_name,
			     64,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve UTF-8 attribute name.",
				 function );

				goto on_error;
			}
			result = libuna_utf8_string_compare_with_utf8_stream(
				  utf8_attribute_name,
				  utf8_attribute_name_size,
				  (uint8_t *) "$I30",
				  4,
				  error );

			if( result == -1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GENERIC,
				 "%s: unable to compare UTF-8 string with $I30.",
				 function );

				return( -1 );
			}
			else if( result == LIBUNA_COMPARE_EQUAL )
			{
				mft_entry->has_i30_index = 1;
			}
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_REPARSE_POINT:
			if( ( mft_entry->reparse_point_attribute_index == -1 )
			 && ( utf8_attribute_name_size <= 1 ) )
			{
				mft_entry->reparse_point_attribute_index = entry_index;
			}
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_SECURITY_DESCRIPTOR:
			if( ( mft_entry->security_descriptor_attribute_index == -1 )
			 && ( utf8_attribute_name_size <= 1 ) )
			{
				mft_entry->security_descriptor_attribute_index = entry_index;
			}
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_STANDARD_INFORMATION:
			if( ( mft_entry->standard_information_attribute_index == -1 )
			 && ( utf8_attribute_name_size <= 1 ) )
			{
				mft_entry->standard_information_attribute_index = entry_index;
			}
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_VOLUME_INFORMATION:
			if( ( mft_entry->volume_information_attribute_index == -1 )
			 && ( utf8_attribute_name_size <= 1 ) )
			{
				mft_entry->volume_information_attribute_index = entry_index;
			}
			break;

		case LIBFSNTFS_ATTRIBUTE_TYPE_VOLUME_NAME:
			if( ( mft_entry->volume_name_attribute_index == -1 )
			 && ( utf8_attribute_name_size <= 1 ) )
			{
				mft_entry->volume_name_attribute_index = entry_index;
			}
			break;

		default:
			break;
	}
	return( 1 );

on_error:
#if defined( HAVE_DEBUG_OUTPUT )
	if( bitmap_values != NULL )
	{
		libfsntfs_bitmap_values_free(
		 &bitmap_values,
		 NULL );
	}
#endif
	return( -1 );
}

/* Appends a $DATA attribute
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_append_data_attribute(
     libfsntfs_mft_entry_t *mft_entry,
     libfsntfs_mft_attribute_t *attribute,
     libcerror_error_t **error )
{
	uint8_t utf8_attribute_name[ 64 ];

	libfsntfs_mft_attribute_t *data_attribute = NULL;
	static char *function                     = "libfsntfs_mft_entry_append_data_attribute";
	size_t utf8_attribute_name_size           = 0;
	int attribute_index                       = 0;
	int entry_index                           = 0;
	int result                                = 0;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_attribute_get_utf8_name_size(
	     attribute,
	     &utf8_attribute_name_size,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve UTF-8 attribute name size.",
		 function );

		return( -1 );
	}
	if( utf8_attribute_name_size <= 1 )
	{
		if( libfsntfs_mft_attribute_append_to_chain(
		     &( mft_entry->data_attribute ),
		     attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
			 "%s: unable to chain attribute.",
			 function );

			return( -1 );
		}
	}
	else
	{
		if( libfsntfs_mft_attribute_get_utf8_name(
		     attribute,
		     utf8_attribute_name,
		     64,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve UTF-8 attribute name.",
			 function );

			return( -1 );
		}
		result = libfsntfs_mft_entry_get_data_attribute_by_utf8_name(
			  mft_entry,
			  utf8_attribute_name,
			  utf8_attribute_name_size,
			  &attribute_index,
			  &data_attribute,
			  error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve data attribute.",
			 function );

			return( -1 );
		}
		else if( result == 0 )
		{
			if( libcdata_array_append_entry(
			     mft_entry->alternate_data_attributes_array,
			     &entry_index,
			     (intptr_t *) attribute,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
				 "%s: unable to append alternate data attribute to array.",
				 function );

				return( -1 );
			}
		}
		else
		{
			if( libfsntfs_mft_attribute_append_to_chain(
			     &data_attribute,
			     attribute,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
				 "%s: unable to chain alternate data attribute.",
				 function );

				return( -1 );
			}
			if( libcdata_array_set_entry_by_index(
			     mft_entry->alternate_data_attributes_array,
			     attribute_index,
			     (intptr_t *) data_attribute,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to append set data attribute: %d in array.",
				 function,
				 attribute_index );

				return( -1 );
			}
		}
	}
	return( 1 );
}

/* Retrieves a data attribute with the specified name
 * Returns 1 if successful, 0 if no attribute was found or -1 on error
 */
int libfsntfs_mft_entry_get_data_attribute_by_utf8_name(
     libfsntfs_mft_entry_t *mft_entry,
     const uint8_t *utf8_string,
     size_t utf8_string_length,
     int *attribute_index,
     libfsntfs_mft_attribute_t **attribute,
     libcerror_error_t **error )
{
	libfsntfs_mft_attribute_t *safe_attribute = NULL;
	static char *function                     = "libfsntfs_mft_entry_get_data_attribute_by_utf8_name";
	int number_of_attributes                  = 0;
	int result                                = 0;
	int safe_attribute_index                  = 0;

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	if( utf8_string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid UTF-8 string.",
		 function );

		return( -1 );
	}
	if( utf8_string_length > (size_t) ( SSIZE_MAX - 1 ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid UTF-8 string length value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( attribute_index == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute index.",
		 function );

		return( -1 );
	}
	if( attribute == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute.",
		 function );

		return( -1 );
	}
	if( libcdata_array_get_number_of_entries(
	     mft_entry->alternate_data_attributes_array,
	     &number_of_attributes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of alternate data attributes.",
		 function );

		return( -1 );
	}
	for( safe_attribute_index = 0;
	     safe_attribute_index < number_of_attributes;
	     safe_attribute_index++ )
	{
		if( libcdata_array_get_entry_by_index(
		     mft_entry->alternate_data_attributes_array,
		     safe_attribute_index,
		     (intptr_t **) &safe_attribute,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve alternate data attribute: %d.",
			 function,
			 safe_attribute_index );

			return( -1 );
		}
		result = libfsntfs_mft_attribute_compare_name_with_utf8_string(
			  safe_attribute,
			  utf8_string,
			  utf8_string_length,
			  error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GENERIC,
			 "%s: unable to compare UTF-8 string with alternative data attribute: %d name.",
			 function,
			 safe_attribute_index );

			return( -1 );
		}
		else if( result != 0 )
		{
			break;
		}
	}
	if( result != 0 )
	{
		*attribute_index = safe_attribute_index;
		*attribute       = safe_attribute;
	}
	else
	{
		*attribute_index = 0;
		*attribute       = NULL;
	}
	return( result );
}

/* Determines if the file entry has the directory entries ($I30) index
 * Returns 1 if the default data stream, 0 if not or -1 on error
 */
int libfsntfs_mft_entry_has_directory_entries_index(
     libfsntfs_mft_entry_t *mft_entry,
     libcerror_error_t **error )
{
	static char *function = "libfsntfs_mft_entry_has_directory_entries_index";

	if( mft_entry == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid MFT entry.",
		 function );

		return( -1 );
	}
	return( (int) mft_entry->has_i30_index );
}

/* Reads the MFT entry
 * Callback function for the MFT entry vector
 * Returns 1 if successful or -1 on error
 */
int libfsntfs_mft_entry_read_element_data(
     libfsntfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfdata_vector_t *vector,
     libfdata_cache_t *cache,
     int element_index,
     int element_data_file_index LIBFSNTFS_ATTRIBUTE_UNUSED,
     off64_t element_data_offset,
     size64_t element_data_size,
     uint32_t element_flags,
     uint8_t read_flags LIBFSNTFS_ATTRIBUTE_UNUSED,
     libcerror_error_t **error )
{
	libfsntfs_mft_entry_t *mft_entry = NULL;
	static char *function            = "libfsntfs_mft_entry_read_element_data";
	uint8_t flags                    = 0;

	LIBFSNTFS_UNREFERENCED_PARAMETER( element_data_file_index )
	LIBFSNTFS_UNREFERENCED_PARAMETER( read_flags )

#if ( SIZEOF_INT <= 4 )
	if( element_index < 0 )
#else
	if( ( element_index < 0 )
	 || ( (int64_t) element_index > (int64_t) UINT32_MAX ) )
#endif
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid element index value out of bounds.",
		 function );

		return( -1 );
	}
	if( element_data_size > (size64_t) UINT32_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid element data size value out of bounds.",
		 function );

		return( -1 );
	}
	if( libfsntfs_mft_entry_initialize(
	     &mft_entry,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create MFT entry.",
		 function );

		goto on_error;
	}
	if( libfsntfs_mft_entry_read_file_io_handle(
	     mft_entry,
	     file_io_handle,
	     element_data_offset,
	     (uint32_t) element_data_size,
	     (uint32_t) element_index,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read MFT entry: %d.",
		 function,
		 element_index );

		goto on_error;
	}
	if( ( element_flags & LIBFSNTFS_MFT_ENTRY_FLAG_MFT_ONLY ) != 0 )
	{
		flags = LIBFSNTFS_FILE_ENTRY_FLAGS_MFT_ONLY;
	}
	if( mft_entry->is_empty == 0 )
	{
		if( libfsntfs_mft_entry_read_attributes(
		     mft_entry,
		     io_handle,
		     file_io_handle,
		     vector,
		     flags,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read MFT entry: %d attributes.",
			 function,
			 element_index );

			goto on_error;
		}
	}
	if( libfdata_vector_set_element_value_by_index(
	     vector,
	     (intptr_t *) file_io_handle,
	     cache,
	     element_index,
	     (intptr_t *) mft_entry,
	     (int (*)(intptr_t **, libcerror_error_t **)) &libfsntfs_mft_entry_free,
	     LIBFDATA_VECTOR_ELEMENT_VALUE_FLAG_MANAGED,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to set MFT entry as element value.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( mft_entry != NULL )
	{
		libfsntfs_mft_entry_free(
		 &mft_entry,
		 NULL );
	}
	return( -1 );
}

