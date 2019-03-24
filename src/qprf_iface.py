from tempfile import NamedTemporaryFile
import json
import subprocess
import os


def print_devices(qprf_binary='qprf'):
    res = subprocess.call([qprf_binary, '--print-devices'])
    return res


def generate_lookup_table(input_filename,
    variable_spec, output_filename,
    x_start, x_end, x_step,
    y_start, y_end, y_step,
    rfsize_start, rfsize_end, rfsize_step,
    device=0, ntasks=512, qprf_binary='qprf',
    delete_spec=True):

    params = dict(locals())
    del params['device']
    del params['ntasks']
    del params['qprf_binary']
    del params['delete_spec']

    spec = {'qprf_job': {'job_type': 'generate_lookup_table',
        'job_params': params } }

    spec_fname = None

    with NamedTemporaryFile(mode='w', delete=False) as f:
        spec_fname = f.name
        json.dump(spec, f)

    res = subprocess.call([qprf_binary, '--run-job', spec_fname,
        '--use-device', str(device), '--num-tasks', str(ntasks)])

    if delete_spec:
        os.unlink(spec_fname)

    return res


def perform_sampling(hrf_model, prf_model, data_file_name,
    data_variable_spec, lookup_table_filename,
    number_of_steps, temperature, x_stdev, y_stdev,
    rfsize_stdev, expt_stdev, gain_stdev,
    subdiv, kappa_stdev, tau_stdev, time_repeat,
    epsilon_stdev, time_echo, gamma_stdev,
    grubb_stdev, rho_stdev, device=0, ntasks=512,
    qprf_binary='qprf', delete_spec=True):

    assert hrf_model in {'5param', '3param', 'canonical'}
    assert prf_model in {'CSS_PRF', 'DUM_WAN'}

    params = dict(locals())
    del params['hrf_model']
    del params['device']
    del params['ntasks']
    del params['qprf_binary']
    del params['delete_spec']

    spec = {'qprf_job': {'job_type': 'perform_sampling_' + hrf_model,
        'job_params': params } }

    spec_fname = None

    with NamedTemporaryFile(mode='w', delete=False) as f:
        spec_fname = f.name
        json.dump(spec, f)

    res = subprocess.call([qprf_binary, '--run-job', spec_fname,
        '--use-device', str(device), '--num-tasks', str(ntasks)])

    if delete_spec:
        os.unlink(spec_fname)

    return res
