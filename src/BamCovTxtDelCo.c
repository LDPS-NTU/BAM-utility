#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "zlib.h"

#include "BamCommonLibrary.h"




void	alignment_CoverageTxtDel(uint8_t *stream, alignmentHeader *AlignmentHeader, posCoverage *PosCoverage, char *chr_bin, uint32_t *Coverage, uint32_t *PolyDelCoverage, uint32_t ref_length);

int	BamCovTxtDel(FILE *file_bam_i, FILE *file_length_o, int flag_hide, char *chr_dir){

	FILE	*file_cov_o;
	FILE	*file_chrbin_i;

	uint8_t *stream_i;//[65536];
	uint8_t *stream_o;//[65536];
	uint8_t	*buffer;
	uint8_t	*top;

	int	i;
	int	len_data;
	int	counter;
	int	ref_ID	= -1;

	uint32_t CRC32;
	uint32_t I_size;

	uint16_t unknown;

	z_stream infstream;
	bgfzTail BgfzTail;
	
	alignmentHeader	AlignmentHeader;
	posCoverage	*PosCoverage;
	uint32_t	*Coverage;
	uint32_t	*PolyDelCoverage;
	char	*ChrBinary;

	char	magic[4];
	char	**chr_name;
	int	*chr_length;
	int32_t	l_text;
	char	*text;
	int32_t	n_ref;
	uint8_t *address;
	char	filename[name_len];
	char	dirname_region[name_len];

	int	flag_bam=0;
	int	flag_outlen=0;
	int	flag_header=0;
	int	flag_cov = 0;
	int	flag_sam = 0;
	int	flag_region = 0;
	int	flag_region_file = 0;
	int	flag_dirname_region= 0;
	int	All_Coverage;

	stream_i= calloc(65536,sizeof(uint8_t));
	stream_o= calloc(65536,sizeof(uint8_t));
	buffer	= calloc(65536*2,sizeof(uint8_t));	


	/* Initial*/
	stream_i[0] = 120;
	stream_i[1] = 156;
	/*---------------*/


	len_data = BGFZBlock(file_bam_i);
	decompressBlock(&infstream, stream_i, stream_o, len_data, file_bam_i);
	fread(&BgfzTail	,sizeof(bgfzTail),1,file_bam_i);

	memcpy(buffer,stream_o,65536);

	address = buffer;	
	memcpy(magic,address,sizeof(char)*4);
	memcpy(&l_text,address+4,sizeof(int32_t));
	text = calloc(l_text+1,sizeof (char));
	memcpy(text,address+8,sizeof(char)*l_text);
	memcpy(&n_ref,address+8+l_text,sizeof(int32_t));

	if (flag_header){
		printf("%d\n%s%d\n",l_text,text,n_ref);	
	}
	address += (12+l_text);
	chr_name	= malloc(n_ref*sizeof(char *));	
	chr_length	= calloc(n_ref,sizeof(int));
	for (i = 0;i < n_ref;i++){
		chr_name[i]	= calloc(name_len,sizeof(char));
		address += refInformation(address,chr_name[i],&chr_length[i]);
	}
	if (file_length_o != NULL){
		for (i = 0;i < n_ref;i++){
			fprintf(file_length_o,"%s\t%d\n",chr_name[i], chr_length[i]);
		}
	}
	if (flag_hide != 1){
		for (i = 0;i < n_ref;i++){
			printf("%s\t%d\n",chr_name[i], chr_length[i]);
		}
	}
	top = buffer + BgfzTail.I_size;
	counter	= top - address;



	//Start Unzip and Process Bam File
	
	while ( (len_data = BGFZBlock(file_bam_i)) > 0){
		decompressBlock(&infstream, stream_i, stream_o, len_data, file_bam_i);
		fread(&BgfzTail	,sizeof(bgfzTail),1,file_bam_i);

		memcpy(buffer,address,sizeof(uint8_t)*counter);
		memcpy(&buffer[counter],stream_o,BgfzTail.I_size);
		top	= buffer + counter + BgfzTail.I_size;
		address = buffer;
		
		if (top - address > sizeof(alignmentHeader)){
			memcpy(&AlignmentHeader, address, sizeof(alignmentHeader));
		}
	
		while ( top - address >= (AlignmentHeader.block_size + 4) ){
			if (AlignmentHeader.refID != ref_ID){
				if (ref_ID != -1){
					if (flag_hide == 0){
						printf("[Bam File Unzip %d / %d ] %s done\n",ref_ID+1,n_ref,chr_name[ref_ID]);
					}	
					for (i = 0;i < chr_length[ref_ID];i++){
						
						if (Coverage[i] != 0){
							All_Coverage = PosCoverage[i].A+ PosCoverage[i].C+ PosCoverage[i].G+ PosCoverage[i].T+ PosCoverage[i].N;
							fprintf(file_cov_o,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
							i+1,
							PosCoverage[i].A,
							PosCoverage[i].C,
							PosCoverage[i].G,
							PosCoverage[i].T,
							PosCoverage[i].N,
							Coverage[i]-All_Coverage,
							Coverage[i],
							PolyDelCoverage[i]);	
						}	
					}
					free(PosCoverage);
					free(Coverage);
					free(PolyDelCoverage);
					free(ChrBinary);
					fclose(file_chrbin_i);
					fclose(file_cov_o);
				}
				if (AlignmentHeader.refID != -1){
					strcat_triple(filename, chr_dir, chr_name[AlignmentHeader.refID], ".bin", name_len);
					file_chrbin_i = fopen(filename,"rb");
					ChrBinary = calloc(chr_length[AlignmentHeader.refID],sizeof(char));
					fread(ChrBinary,chr_length[AlignmentHeader.refID],sizeof(char),file_chrbin_i);
					
					PosCoverage = calloc(chr_length[AlignmentHeader.refID],sizeof(posCoverage));
					Coverage = calloc(chr_length[AlignmentHeader.refID],sizeof(uint32_t));
					PolyDelCoverage = calloc(chr_length[AlignmentHeader.refID],sizeof(uint32_t));
					strcat_triple(filename, "", chr_name[AlignmentHeader.refID], "_cov.txt", name_len);
					file_cov_o = fopen(filename,"w");
				}
				ref_ID = AlignmentHeader.refID;
			}
			if (ref_ID == -1){	break;	}
			address += sizeof(alignmentHeader);
		
			//Core_START
			alignment_CoverageTxtDel(address, &AlignmentHeader, PosCoverage, ChrBinary, Coverage, PolyDelCoverage, chr_length[ref_ID]);
			//Core_END
			
			address += (AlignmentHeader.block_size + 4) - sizeof(alignmentHeader);

			if (top - address > sizeof(alignmentHeader)){
				memcpy(&AlignmentHeader, address, sizeof(alignmentHeader));
			}else {
				break;	
			}
		}
		counter = top - address;
		
		if (ref_ID == -1){	break;	}
	}
	if (ref_ID != -1){
		if (flag_hide == 0){
			printf("[Bam File Unzip %d / %d ] %s done\n",ref_ID+1,n_ref,chr_name[ref_ID]);
		}
		for (i = 0;i < chr_length[ref_ID];i++){
			if (Coverage[i] != 0){
				All_Coverage = PosCoverage[i].A+ PosCoverage[i].C+ PosCoverage[i].G+ PosCoverage[i].T+ PosCoverage[i].N;
				fprintf(file_cov_o,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
				i+1,
				PosCoverage[i].A,
				PosCoverage[i].C,
				PosCoverage[i].G,
				PosCoverage[i].T,
				PosCoverage[i].N,
				Coverage[i] - All_Coverage,
				Coverage[i],
				PolyDelCoverage[i]);	
			}	
		}
		free(PosCoverage);
		free(Coverage);
		free(PolyDelCoverage);
		free(ChrBinary);
		fclose(file_cov_o);
		fclose(file_chrbin_i);
	}
	
	free(stream_i);	
	free(stream_o);	
	free(buffer);	
	return 0;
}

void	alignment_CoverageTxtDel(uint8_t *stream, alignmentHeader *AlignmentHeader, posCoverage *PosCoverage, char *chr_bin, uint32_t *Coverage, uint32_t *PolyDelCoverage, uint32_t ref_length){

	int	i,j;

	char	*read_name;
	uint32_t *cigar;
	uint8_t	*seq;
	char	*qual;

	int	op;
	int	op_len;
	char	residue;
	int	length;
	int	index;

	MemoryCopy(&stream,(void **)&read_name	, AlignmentHeader->l_read_name	, sizeof(char));
	MemoryCopy(&stream,(void **)&cigar	, AlignmentHeader->n_cigar_op	, sizeof(uint32_t));
	MemoryCopy(&stream,(void **)&seq	, (AlignmentHeader->l_seq+1)/2	, sizeof(uint8_t));

	length = 0;
	index = AlignmentHeader->pos;
	for (i = 0;i < AlignmentHeader->n_cigar_op;i++){
		op	= cigar[i] & 7;
		op_len	= cigar[i] >> 4;
		if (CIGAR(op) == 'M'){
			if (index + op_len > ref_length){
				op_len = ref_length - index;
			}
			for (j = 0;j < op_len;j++){
				residue = Bin2SeqTop(seq[length >> 1],length&1);
				if (residue == 'A'){		PosCoverage[index].A++;
				}else if (residue == 'T'){	PosCoverage[index].T++;
				}else if (residue == 'G'){	PosCoverage[index].G++;
				}else if (residue == 'C'){	PosCoverage[index].C++;
				}else if (residue == 'N'){	PosCoverage[index].N++;
				}else {
					printf("Error: Residue:%c\n",residue);
					exit(1);
				}
				Coverage[index]++;
				length++;
				index++;
			}
		}else if (CIGAR(op) == 'I'){
			length += op_len;
		}else if (CIGAR(op) == 'D'){
			if (op_len == 1){
				if ((Bin2SeqTop(seq[(length-1) >> 1],(length-1)&1) == chr_bin[index] && chr_bin[index] == chr_bin[index-1])|| 
				(Bin2SeqTop(seq[length >> 1],length&1)== chr_bin[index] && chr_bin[index] == chr_bin[index+1])){
					if (chr_bin[index] == 'A'){		PosCoverage[index].A++;
					}else if (chr_bin[index] == 'C'){	PosCoverage[index].C++;
					}else if (chr_bin[index] == 'G'){	PosCoverage[index].G++;
					}else if (chr_bin[index] == 'T'){	PosCoverage[index].T++;
					}else if (chr_bin[index] == 'N'){	PosCoverage[index].N++;
					}
					PolyDelCoverage[index]++;
				}
				Coverage[index]++;
				index++;
			}else {
				for (j = 0;j < op_len;j++){
					Coverage[index]++;
					index++;
				}
			}
		}else if (CIGAR(op) == 'S'){
			length += op_len;
		}else if (CIGAR(op) == 'N'){
			index += op_len;
		}
		// CIGAR(op) == 'H' don't care about it.
		
	}
	
	free(read_name);
	free(cigar);
	free(seq);
}




